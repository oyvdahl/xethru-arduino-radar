/*
  XeThruRadar.cpp - Library for using the XeThru Radar module
  --Created by Oyvind N. Dahl, August 13, 2015.
  --Updated by Oyvind N. Dahl, May 22, 2017.
  
  Notes:
  --Sleep application not implemented
  --Do not use enableDebug() function if you want to use the same serial port in your application
  --Designed for Arduino Zero. To use with other Arduinos, edit the line "#define SerialRadar Serial1" below
  --To use the enableDebug() function with other Arduinos, edit the line "#define SerialDebug SerialUSB" below
  
*/

#include "XeThruRadar.h"

#ifndef SERIAL_BUFFER_SIZE
#define SERIAL_BUFFER_SIZE SERIAL_RX_BUFFER_SIZE
#endif

#define SerialRadar Serial1
#define SerialDebug SerialUSB


XeThruRadar::XeThruRadar() {}

void XeThruRadar::init()
{
  debug_println("Init sequence starting...");
  
  // Init serial port for communicating with radar
  SerialRadar.begin(115200);
  delay(100);
  
  // Empty incoming serial buffer (in case of old data)
  empty_serial_buffer();
  
  // Reset module to start from scratch
  reset_module();

  // Receive system status messages
  debug_println("Waiting for system status messages...");
  while (!radar_ready()) {
    debug_println("Radar not ready, trying another reset...");
    empty_serial_buffer();
    send_command(&_xts_spc_mod_reset, 1);
  }
  
  debug_println("Init sequence complete!");
}



/*********************************************** 
  For enabling internal debugging
  
  Note: should only be used by developers 
        to debugging/improving the library
 ***********************************************/
void XeThruRadar::enableDebug()
{
  enable_debug_port = true;
  SerialDebug.begin(115200);
  while (!SerialDebug);
  
  debug_println("Debugging enabled");
  debug_println("Waiting 5 seconds");
  delay(5000);
}






/*************************************
*
* SEND / RECEIVE / SERIAL COMMANDS
*
*************************************/


/**
* Reads one character from the serial RX buffer.
* Blocks until a character is available.
*/
unsigned char XeThruRadar::serial_read_blocking()
{
  while (SerialRadar.available() < 1) {
    delay(1);
  }
  return SerialRadar.read();
}


/**
* Checks if the RX buffer is overflowing.
* The Arduino RX buffer is only 64 bytes,
* so this happens a lot with fast data rates
*/
bool XeThruRadar::check_overflow()
{
  if (SerialRadar.available() >= SERIAL_BUFFER_SIZE-1) {
    debug_println("Buffer overflowed");
    return true;
  }
  
  return false;
}


// Empties the Serial RX buffer
void XeThruRadar::empty_serial_buffer()
{
  debug_print("Emptying serial buffer... ");
  
  while (SerialRadar.available() > 0)
    SerialRadar.read();  // Remove one byte from the buffer
  
  debug_println("Done!");
}



///////////////////
// DEBUG functions
///////////////////
void XeThruRadar::debug_println(String msg)
{
  if (enable_debug_port == true)
    SerialDebug.println(msg);
}

void XeThruRadar::debug_print(String msg)
{
  if (enable_debug_port == true)
    SerialDebug.print(msg);
}




/*****************
* Sends a command
******************/
void XeThruRadar::send_command(const unsigned char * cmd, int len) {

  // Calculate CRC
  char crc = _xt_start;
  for (int i = 0; i < len; i++)
    crc ^= cmd[i];


  // Add escape bytes if necessary
  for (int i = 0; i < len; i++) {
    if (cmd[i] == 0x7D || cmd[i] == 0x7E || cmd[i] == 0x7F)
    {
     //TODO: Implement escaping
     SerialDebug.write("CRC Escaping needed for send:buf! Halting...");
     while(1) {}
    }
  }

  // Send _xt_start + command + crc_string + _xt_stop
  SerialRadar.write(_xt_start);
  SerialRadar.write(cmd, len);
  SerialRadar.write(crc);
  SerialRadar.write(_xt_stop);
  SerialRadar.flush();
  
}
  
  
/**
* Receives one data package from Serial
* buffer and stores it in recv_buf
*
* Returns the length of the data received.
*/ 
int XeThruRadar::receive_data(bool print_data) {
  
  int last_char = 0;
  int recv_len = 0;  //Number of bytes received
  unsigned char cur_char;

  //Wait for start character
  while (1)
  {
    // Check if input buffer is overflowed
    if (check_overflow())
      empty_serial_buffer();

    // Get one byte from radar
    cur_char = serial_read_blocking();
  
    if (cur_char == _xt_escape)
    {
      // Check if input buffer is overflowed
      if (check_overflow()) {
        debug_println("Overflow while receiving. Aborting receive_data()");
        return -1;
      }  

      // If it's an escape character –
      // ...ignore next character in buffer
      serial_read_blocking();
    }
    else if (cur_char == _xt_start)
    {
      // If it's the start character –
      // ...we fill the first character of the buffer and move on
      _recv_buf[0] = _xt_start;
      recv_len = 1;
      break;
    }
  }

  // Start receiving the rest of the bytes
  while (1)
  {
    // Check if input buffer is overflowed
    if (check_overflow()) {
      debug_println("Overflow while receiving. Aborting receive_data()");
      return -1;
    }

    // Get one byte from radar
    cur_char = serial_read_blocking();  

    if (cur_char == _xt_escape)
    {
      // Check if input buffer is overflowed
      if (check_overflow()) {
        debug_println("Overflow while receiving. Aborting receive_data()");
        return -1;
      }
    
      // If it's an escape character –
      // fetch the next byte from serial
      cur_char = serial_read_blocking();
  
      // Make sure to not overwrite receive buffer
      if (recv_len >= RX_BUF_LENGTH) {
        debug_println("Received more than rx buffer size. Aborting receive_data()");
        return -1;
      }
        

      // Fill response buffer, and increase counter
      _recv_buf[recv_len] = cur_char;
      recv_len++;
    }

    else if (cur_char == _xt_start)
    {
      // If it's the start character, something is wrong
      debug_println("Start character received in the middle of message. Aborting receive_data()");
      return -1;
    }

    else
    {
      // Make sure not overwrite receive buffer
      if (recv_len >= RX_BUF_LENGTH)
        break;

      // Fill response buffer, and increase counter
      _recv_buf[recv_len] = cur_char;
      recv_len++;
  
      // is it the stop byte?
      if (cur_char == _xt_stop) {
        break;  //Exit this loop
      }
    }
  }


  // Calculate CRC
  char crc = 0;
  char escape_found = 0;

  // CRC is calculated without the crc itself and the stop byte, hence the -2 in the counter
  for (int i = 0; i < recv_len-2; i++)
  {
    crc ^= _recv_buf[i];
    escape_found = 0;
  }


  // Print the received data
   if (print_data) {
    SerialDebug.print("Data: ");
    for (int i = 0; i < recv_len; i++) {
      SerialDebug.print(_recv_buf[i] ,HEX);
      SerialDebug.print(" ");
    }
    SerialDebug.println(" ");
  }


  // Check if calculated CRC matches the recieved
  if (crc == _recv_buf[recv_len-2])  {
    return recv_len;  // Return length of data packet upon success
  }
  else {
    debug_println("[Error]: CRC check failed!");
    return -1; // Return -1 upon crc failure
  }

}









/*********************
*
* PROTOCOL COMMANDS
*
*********************/

/**
* Waiting for radar to become ready in the bootup sequence
*/
bool XeThruRadar::radar_ready()
{
  // Try receiving xts_sprs_ready signal up to 5 times
  for (int i = 0; i < 5; i++) {
    receive_data(enable_debug_port);
    if (_recv_buf[2] == _xts_sprs_ready) {
      return true;
    }
    delay(500);
  }

  return false;
}


bool XeThruRadar::ping_radar()
{
  unsigned long pong_val = 0;
  
  debug_println("About to ping radar");
  
  //Fill send buffer
  _send_buf[0] = _xts_spc_ping;
  memcpy(_send_buf+1, &_xts_def_pingval, 4);

  debug_println("ping_radar: memory ready");
  //Send the command
  send_command(_send_buf, 5);

  debug_println("ping_radar: sent. Waiting for reply");
  //Get response
  receive_data(enable_debug_port);

  debug_println("ping_radar: analysing response");
  // Get the returned pong value
  get_pong_val(&pong_val);

  //Check the pong value
  if (pong_val == _xts_def_pongval_ready) {
    debug_println("ping_radar: PONGVAL_READY recieved");
    return true;
  }
  else if (pong_val == _xts_def_pongval_notready) {
    debug_println("ping_radar: PONGVAL_NOT_READY recieved");
    return false;
  }
  else {
    debug_println("Unknown PONG value");
    return false;
  }
}

void XeThruRadar::get_pong_val(unsigned long * pong_val)
{ 
  // Check that the received message is a PONG response
  if (_recv_buf[1] != _xts_spr_pong) {
    debug_println("Expected PONG, received something else. Halting.");
    while (1) {}
  }
 
  // Get the pong_val of the response
  memcpy(pong_val, _recv_buf+2, 4);
}


// RESET MODULE
void XeThruRadar::reset_module() {
  
  debug_println("Resetting module..."); 
  send_command(&_xts_spc_mod_reset, 1);
  
  //TODO: Implement some error checking to see that we recieve the correct response
  receive_data(enable_debug_port);
  receive_data(enable_debug_port);
}


// Load respiration app
void XeThruRadar::load_respiration_app() 
{
  //Fill send buffer
  _send_buf[0] = _xts_spc_mod_loadapp;
  _send_buf[4] = (_xts_id_app_resp >> 24) & 0xff;
  _send_buf[3] = (_xts_id_app_resp >> 16) & 0xff;  
  _send_buf[2] = (_xts_id_app_resp >> 8) & 0xff;
  _send_buf[1] = _xts_id_app_resp & 0xff;
  
  //Send the command
  send_command(_send_buf, 5);
  
  //Get response
  receive_data(enable_debug_port);
}

// Execute application
void XeThruRadar::execute_app() 
{
  //Fill send buffer
  _send_buf[0] = _xts_spc_mod_setmode;
  _send_buf[1] = _xts_sm_run;

  // Send the command
  send_command(_send_buf, 2);
  // Get response
  receive_data(enable_debug_port);  
}


/**
* Set the detection zone of the radar
*/
void XeThruRadar::setDetectionZone(float start_zone, float end_zone)
{
  _send_buf[0] = _xts_spc_appcommand;
  _send_buf[1] = _xts_spca_set;

  memcpy(_send_buf+2, &_xts_id_detection_zone, 4);
  memcpy(_send_buf+6, &start_zone, 4);
  memcpy(_send_buf+10, &end_zone, 4);

  //Send the command
  send_command(_send_buf, 14);

  //Get response
  receive_data(enable_debug_port);

  // Check if acknowledge was received
  //check_ack();
}


/**
* Set sensitivity
*/
void XeThruRadar::setSensitivity(long sensitivity)
{
  _send_buf[0] = _xts_spc_appcommand;
  _send_buf[1] = _xts_spca_set;

  memcpy(_send_buf+2, &_xts_id_sensitivity, 4);
  memcpy(_send_buf+6, &sensitivity, 4);

  //Send the command
  send_command(_send_buf, 10);

  //Get response
  receive_data(enable_debug_port);

  // Check if acknowledge was received
  //check_ack();

}




/**********************************************************
  This function retrieves a packet of respiration data, 
  extracts the content and returns it in a readable format
***********************************************************/
RespirationData XeThruRadar::get_respiration_data()
{
  RespirationData data; // For storing and returning respiration data
  String str;           //For debugging
  
  // receive_data() fills _recv_buf[] with valid data
  if (receive_data(enable_debug_port) < 0)
  {
     //Something went wrong! 
     data.valid_data = false;
     return data;
  }
  
  // Check that it's app-data we've received
  if (_recv_buf[1] != _xts_spr_appdata)
  {
     //Something went wrong! 
     data.valid_data = false;
     return data;
  }
  
  // Get state code
  data.state_code = _recv_buf[10];
  str = String((int)data.state_code);
  debug_println("get_respiration_data: state_code= " + str);
  
  // Get rpm value
  memcpy(&data.rpm, &_recv_buf[14], 4);
  //data.rpm = *(int*)&_recv_buf[14];
  str = String(data.rpm);
  debug_println("get_respiration_data: rpm= " + str);
  
  // Get movement value
  memcpy(&data.movement, &_recv_buf[22], 4);
  //data.movement = *(float*)&_recv_buf[22];
  str = String(data.movement);
  debug_println("get_respiration_data: movement= " + str);
  
  // Set state to indicate valid data
  data.valid_data = true;
  
  // Return the extracted data
  return data;
}


