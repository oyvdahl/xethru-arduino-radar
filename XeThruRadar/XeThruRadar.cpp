<<<<<<< HEAD
/*
  XeThruRadar.cpp - Library for using the XeThru Radar module
  Created by Oyvind N. Dahl, August 13, 2015.
*/

#include "XeThruRadar.h"

/*
XeThruRadar::XeThruRadar()
{
  Serial.begin(115200);
  Serial.write(88);
}
*/

XeThruRadar::XeThruRadar()
{
}

void XeThruRadar::init()
{
  Serial.begin(115200); 
}


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
    }
  }
  
  // Send xt_start + command + crc_string + xt_stop
  Serial.write(_xt_start);
  Serial.write(cmd, len);
  Serial.write(crc);
  Serial.write(_xt_stop);
  
}
  
  
  
int XeThruRadar::receive_data() {
  
  // Get response

  char last_char = 0x00;
  int recv_len = 0;  //Number of bytes received

  //Wait for start character
  while (1) 
  {
    char c = Serial.read();	// Get one byte from radar
    
    if (c == _xt_escape)
    {
      // If it's an escape character –
      // ...ignore next character in buffer
      Serial.read();
    }
    else if (c == _xt_start) 
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
    // read a byte
    char cur_char = Serial.read();	// Get one byte from radar
    
    if (cur_char == -1) {
      continue;
    }
      
    // Fill response buffer, and increase counter
    _recv_buf[recv_len] = cur_char;
    recv_len++;
    
    // is it the stop byte?
    if (cur_char == _xt_stop) {
      if (last_char != _xt_escape)
        break;  //Exit this loop 
    }
    
    // Update last_char
    last_char = cur_char;
  }
  
  
  
  // Calculate CRC
  char crc = 0;
  char escape_found = 0;
  
  // CRC is calculated without the crc itself and the stop byte, hence the -2 in the counter
  for (int i = 0; i < recv_len-2; i++) 
  {
    // We need to ignore escape bytes when calculating crc
    if (_recv_buf[i] == _xt_escape && !escape_found) {
      escape_found = 1;
      continue;
    }
    else {
      crc ^= _recv_buf[i];
      escape_found = 0;
    }
  }
  
  
  // Check if calculated CRC matches the recieved
  if (crc == _recv_buf[recv_len-2]) 
  {
    return 0;  // Return 0 upon success
  }
  else 
  {
    return -1; // Return -1 upon crc failure
  } 
}

int XeThruRadar::receive_raw_data() {
  
  // Get response

  char last_char = 0x00;
  int recv_len = 0;  //Number of bytes received

  //Wait for start character
  while (1) 
  {
    char c = Serial.read();	// Get one byte from radar
    
    if (c == _xt_escape)
    {
      // If it's an escape character –
      // ...ignore next character in buffer
      Serial.read();
    }
    else if (c == _xt_start) 
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
    // read a byte
    char cur_char = Serial.read();	// Get one byte from radar
    
    if (cur_char == -1) {
      continue;
    }
      
    // Fill response buffer, and increase counter
    _recv_buf[recv_len] = cur_char;
    recv_len++;
    
    // is it the stop byte?
    if (cur_char == _xt_stop) {
      if (last_char != _xt_escape)
        break;  //Exit this loop 
    }
    
    // Update last_char
    last_char = cur_char;
  }
  
  
  
  // Calculate CRC
  char crc = 0;
  char escape_found = 0;
  
  // CRC is calculated without the crc itself and the stop byte, hence the -2 in the counter
  for (int i = 0; i < recv_len-2; i++) 
  {
    // We need to ignore escape bytes when calculating crc
    if (_recv_buf[i] == _xt_escape && !escape_found) {
      escape_found = 1;
      continue;
    }
    else {
      crc ^= _recv_buf[i];
      escape_found = 0;
    }
  }
  
  
  // Check if calculated CRC matches the recieved
  if (crc == _recv_buf[recv_len-2]) 
  {
    return recv_len;  // Return 0 upon success
  }
  else 
  {
    return -1; // Return -1 upon crc failure
  } 
}


void XeThruRadar::empty_serial_buffer()
{
  while (Serial.available())
  {
    Serial.read();	// Remove one byte from the buffer
  } 
}


int XeThruRadar::get_raw() {
  
    //The arduino is not fast enough to receive all the data from the radar (it seems)
  //...so always empty the buffer of old data, so that we are sure we have fresh data
  empty_serial_buffer();
  if (receive_raw_data() == -1)
  {
     //Something went wrong! 
     return -1;
  }
  // Now recv_buf should be filled with valid data
  
  // Check that it's app-data we've received
  if (_recv_buf[1] != _xts_spr_appdata)
  {
     //Something went wrong! 
     return -1;
  }
  
  return 66;
}



int XeThruRadar::get_rpm() {
  
  int * rpm;
  
  //The arduino is not fast enough to receive all the data from the radar (it seems)
  //...so always empty the buffer of old data, so that we are sure we have fresh data
  empty_serial_buffer();
  
  if (receive_data() != 0)
  {
     //Something went wrong! 
     return -1;
  }
  
  // Now recv_buf should be filled with valid data
  
  
  // Check that it's app-data we've received
  if (_recv_buf[1] != _xts_spr_appdata)
  {
     //Something went wrong! 
     return -1;
  }
  
  
  // TODO: Check that _xts_id_resp_status is correct (just to make sure we are getting the right data)
  

  // Check that it's the correct state code
  if (_recv_buf[10] != _xts_val_resp_state_breathing)
    return -1;
  
  
  // Breathing detected, extract the rpm value
  rpm = (int*)&_recv_buf[14];

  //Return the rpm value as int
  return *rpm;
}


float XeThruRadar::get_resp_movement() {
  
  float * movement;
  
  //The arduino is not fast enough to receive all the data from the radar (it seems)
  //...so always empty the buffer of old data, so that we are sure we have fresh data
  empty_serial_buffer();
  
  if (receive_data() != 0)
  {
     //Something went wrong! 
     return -99.0;
  }
  
  // Now recv_buf should be filled with valid data
  
  
  // Check that it's app-data we've received
  if (_recv_buf[1] != _xts_spr_appdata)
  {
     //Something went wrong! 
     return -99.0;
  }
  
  // TODO: Check that _xts_id_resp_status is correct (just to make sure we are getting the right data)

  // Check that it's the correct state code
  if (_recv_buf[10] != _xts_val_resp_state_breathing)
    return -99.0; 

  //Point float pointer to the first byte of the float in the buffer
  movement = (float*)&_recv_buf[22];      

  //Return the float value
  return *movement;   
}


int XeThruRadar::get_resp_state() {
  
  //The arduino is not fast enough to receive all the data from the radar (it seems)
  //...so always empty the buffer of old data, so that we are sure we have fresh data
  empty_serial_buffer();
  
  if (receive_data() != 0)
  {
     //Something went wrong! 
     return -1;
  }
  
  // Now recv_buf should be filled with valid data
  
  // Check that it's app-data we've received
  if (_recv_buf[1] != _xts_spr_appdata)
  {
     //Something went wrong! 
     return -1;
  }
  
  // TODO: Check that _xts_id_resp_status is correct (just to make sure we are getting the right data)

  
  // State code
  int state_code = _recv_buf[10]; 
  return state_code;
}



// RESET MODULE
void XeThruRadar::reset_module() {
  send_command(&_xts_spc_mod_reset, 1);
  
  //TODO: Implement some error checking to see that we recieve the correct response
  receive_data();
  receive_data();
}

// Load respiration app
void XeThruRadar::load_respiration_app() 
{
  //Fill send buffer
  unsigned char send_buf[5];
  send_buf[0] = _xts_spc_mod_loadapp;
  send_buf[4] = (_xts_id_app_resp >> 24) & 0xff;
  send_buf[3] = (_xts_id_app_resp >> 16) & 0xff;  
  send_buf[2] = (_xts_id_app_resp >> 8) & 0xff;
  send_buf[1] = _xts_id_app_resp & 0xff;
  
  //Send the command
  send_command(send_buf, 5);
  
  //Get response
  receive_data();
}

// Load sleep app
void XeThruRadar::load_sleep_app() 
{
  //Fill send buffer
  unsigned char send_buf[5];
  send_buf[0] = _xts_spc_mod_loadapp;
  send_buf[4] = (_xts_id_app_sleep >> 24) & 0xff;
  send_buf[3] = (_xts_id_app_sleep >> 16) & 0xff;  
  send_buf[2] = (_xts_id_app_sleep >> 8) & 0xff;
  send_buf[1] = _xts_id_app_sleep & 0xff;
  
  //Send the command
  send_command(send_buf, 5);
  
  //Get response
  receive_data();
}

// Enable Raw Data
void XeThruRadar::enable_raw_data() 
{
  //Convert these values to int
  int XTS_SACR_OUTPUTBASEBAND = _xts_sacr_outputbaseband;
  int XTS_SACR_ID_BASEBAND_OUTPUT_AP = _xts_sacr_id_baseband_output_iq_amplitude_phase;
  
  //Fill send buffer
  unsigned char send_buf[10];
  
  send_buf[0] = _xts_spc_dir_command;
  send_buf[1] = _xts_sdc_app_setint;
  
  send_buf[2] = XTS_SACR_OUTPUTBASEBAND & 0xff;
  send_buf[3] = (XTS_SACR_OUTPUTBASEBAND >> 8) & 0xff;
  send_buf[4] = (XTS_SACR_OUTPUTBASEBAND >> 16) & 0xff;  
  send_buf[5] = (XTS_SACR_OUTPUTBASEBAND >> 24) & 0xff;
  
  send_buf[6] = XTS_SACR_ID_BASEBAND_OUTPUT_AP & 0xff;
  send_buf[7] = (XTS_SACR_ID_BASEBAND_OUTPUT_AP >> 8) & 0xff;
  send_buf[8] = (XTS_SACR_ID_BASEBAND_OUTPUT_AP >> 16) & 0xff;  
  send_buf[9] = (XTS_SACR_ID_BASEBAND_OUTPUT_AP >> 24) & 0xff;  
  
  
  //Send the command
  send_command(send_buf, 10);
  
  //Get response
  receive_data();
}



// Execute application
void XeThruRadar::execute_app() 
{
  //Fill send buffer
  unsigned char send_buf[2];
  send_buf[0] = _xts_spc_mod_setmode;
  send_buf[1] = _xts_sm_run;

  // Send the command
  send_command(send_buf, 2);
  // Get response
  receive_data();  
}

=======
/*
  XeThruRadar.cpp - Library for using the XeThru Radar module
  Created by Oyvind N. Dahl, August 13, 2015.
*/

#include "XeThruRadar.h"

/*
XeThruRadar::XeThruRadar()
{
  Serial.begin(115200);
  Serial.write(88);
}
*/

XeThruRadar::XeThruRadar()
{
}

void XeThruRadar::init()
{
  Serial.begin(115200); 
}


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
    }
  }
  
  // Send xt_start + command + crc_string + xt_stop
  Serial.write(_xt_start);
  Serial.write(cmd, len);
  Serial.write(crc);
  Serial.write(_xt_stop);
  
}
  
  
  
int XeThruRadar::receive_data() {
  
  // Get response

  char last_char = 0x00;
  int recv_len = 0;  //Number of bytes received

  //Wait for start character
  while (1) 
  {
    char c = Serial.read();	// Get one byte from radar
    
    if (c == _xt_escape)
    {
      // If it's an escape character –
      // ...ignore next character in buffer
      Serial.read();
    }
    else if (c == _xt_start) 
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
    // read a byte
    char cur_char = Serial.read();	// Get one byte from radar
    
    if (cur_char == -1) {
      continue;
    }
      
    // Fill response buffer, and increase counter
    _recv_buf[recv_len] = cur_char;
    recv_len++;
    
    // is it the stop byte?
    if (cur_char == _xt_stop) {
      if (last_char != _xt_escape)
        break;  //Exit this loop 
    }
    
    // Update last_char
    last_char = cur_char;
  }
  
  
  
  // Calculate CRC
  char crc = 0;
  char escape_found = 0;
  
  // CRC is calculated without the crc itself and the stop byte, hence the -2 in the counter
  for (int i = 0; i < recv_len-2; i++) 
  {
    // We need to ignore escape bytes when calculating crc
    if (_recv_buf[i] == _xt_escape && !escape_found) {
      escape_found = 1;
      continue;
    }
    else {
      crc ^= _recv_buf[i];
      escape_found = 0;
    }
  }
  
  
  // Check if calculated CRC matches the recieved
  if (crc == _recv_buf[recv_len-2]) 
  {
    return 0;  // Return 0 upon success
  }
  else 
  {
    return -1; // Return -1 upon crc failure
  } 
}


void XeThruRadar::empty_serial_buffer()
{
  while (Serial.available())
  {
    Serial.read();	// Remove one byte from the buffer
  } 
}


int XeThruRadar::get_rpm() {
  
  int * rpm;
  
  //The arduino is not fast enough to receive all the data from the radar (it seems)
  //...so always empty the buffer of old data, so that we are sure we have fresh data
  empty_serial_buffer();
  
  if (receive_data() != 0)
  {
     //Something went wrong! 
     return -1;
  }
  
  // Now recv_buf should be filled with valid data
  
  
  // Check that it's app-data we've received
  if (_recv_buf[1] != _xts_spr_appdata)
  {
     //Something went wrong! 
     return -1;
  }
  
  
  // TODO: Check that _xts_id_resp_status is correct (just to make sure we are getting the right data)
  

  // Check that it's the correct state code
  if (_recv_buf[10] != _xts_val_resp_state_breathing)
    return -1;
  
  
  // Breathing detected, extract the rpm value
  rpm = (int*)&_recv_buf[14];

  //Return the rpm value as int
  return *rpm;
}


float XeThruRadar::get_resp_movement() {
  
  float * movement;
  
  //The arduino is not fast enough to receive all the data from the radar (it seems)
  //...so always empty the buffer of old data, so that we are sure we have fresh data
  empty_serial_buffer();
  
  if (receive_data() != 0)
  {
     //Something went wrong! 
     return -99.0;
  }
  
  // Now recv_buf should be filled with valid data
  
  
  // Check that it's app-data we've received
  if (_recv_buf[1] != _xts_spr_appdata)
  {
     //Something went wrong! 
     return -99.0;
  }
  
  // TODO: Check that _xts_id_resp_status is correct (just to make sure we are getting the right data)

  // Check that it's the correct state code
  if (_recv_buf[10] != _xts_val_resp_state_breathing)
    return -99.0; 

  //Point float pointer to the first byte of the float in the buffer
  movement = (float*)&_recv_buf[22];      

  //Return the float value
  return *movement;   
}


int XeThruRadar::get_resp_state() {
  
  //The arduino is not fast enough to receive all the data from the radar (it seems)
  //...so always empty the buffer of old data, so that we are sure we have fresh data
  empty_serial_buffer();
  
  if (receive_data() != 0)
  {
     //Something went wrong! 
     return -1;
  }
  
  // Now recv_buf should be filled with valid data
  
  // Check that it's app-data we've received
  if (_recv_buf[1] != _xts_spr_appdata)
  {
     //Something went wrong! 
     return -1;
  }
  
  // TODO: Check that _xts_id_resp_status is correct (just to make sure we are getting the right data)

  
  // State code
  int state_code = _recv_buf[10]; 
  return state_code;
}



// RESET MODULE
void XeThruRadar::reset_module() {
  send_command(&_xts_spc_mod_reset, 1);
  
  //TODO: Implement some error checking to see that we recieve the correct response
  receive_data();
  receive_data();
}

// Load respiration app
void XeThruRadar::load_respiration_app() 
{
  //Fill send buffer
  unsigned char send_buf[5];
  send_buf[0] = _xts_spc_mod_loadapp;
  send_buf[4] = (_xts_id_app_resp >> 24) & 0xff;
  send_buf[3] = (_xts_id_app_resp >> 16) & 0xff;  
  send_buf[2] = (_xts_id_app_resp >> 8) & 0xff;
  send_buf[1] = _xts_id_app_resp & 0xff;
  
  //Send the command
  send_command(send_buf, 5);
  
  //Get response
  receive_data();
}

// Execute application
void XeThruRadar::execute_app() 
{
  //Fill send buffer
  unsigned char send_buf[2];
  send_buf[0] = _xts_spc_mod_setmode;
  send_buf[1] = _xts_sm_run;

  // Send the command
  send_command(send_buf, 2);
  // Get response
  receive_data();  
}

>>>>>>> 7f90551a4d224be945a5e3e63edeb2efac8f774d
