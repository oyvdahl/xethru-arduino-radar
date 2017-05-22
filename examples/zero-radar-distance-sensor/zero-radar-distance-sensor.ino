// Written by Øyvind Nydal Dahl
// www.build-electronic-circuits.com
// August 2016



// Radar constants
const unsigned char xt_start = 0x7D;
const unsigned char xt_stop = 0x7E;
const unsigned char xt_escape = 0x7F;

const unsigned char xts_spc_appcommand = 0x10;
const unsigned char xts_spc_mod_setmode = 0x20;
const unsigned char xts_spc_mod_loadapp = 0x21;
const unsigned char xts_spc_mod_reset = 0x22;
const unsigned char xts_spc_mod_setledcontrol = 0x24;

const unsigned char xts_spca_set = 0x10;

const unsigned char xts_spr_appdata = 0x50;
const unsigned char xts_spr_system = 0x30;
const unsigned char xts_spr_ack = 0x10;

const unsigned char xts_spc_dir_command = 0x90;
const unsigned char xts_sdc_app_setint = 0x71;
const unsigned char xts_sdc_comm_setbaudrate = 0x80;

const unsigned long xts_id_detection_zone = 0x96a10a1c;
const unsigned long xts_id_sensitivity = 0x10a5112b;

const unsigned long xts_id_app_resp = 0x1423a2d6;
const unsigned long xts_id_app_sleep = 0x00f17b17;
const unsigned long xts_id_resp_status = 0x2375fe26;
const unsigned long xts_id_sleep_status = 0x2375a16c;

const unsigned long xts_id_baseband_iq = 0x0000000c;
const unsigned long xts_id_baseband_amplitude_phase = 0x0000000d;

const unsigned long xts_sacr_outputbaseband = 0x00000010;
const unsigned long xts_sacr_id_baseband_output_off = 0x00000000;
const unsigned long xts_sacr_id_baseband_output_amplitude_phase = 0x00000002;

const unsigned char xts_sm_run = 0x01;
const unsigned char xts_sm_normal = 0x10;
const unsigned char xts_sm_idle = 0x11;

const unsigned char xts_sprs_booting = 0x10;
const unsigned char xts_sprs_ready = 0x11;


#define BASEBAND_NUM_OF_BINS_MAX 52
#define RX_BUF_LENGTH 512
#define TX_BUF_LENGTH 48

unsigned char recv_buf[RX_BUF_LENGTH]; // Buffer for receiving data from radar
unsigned char send_buf[TX_BUF_LENGTH]; // Buffer for sending data to radar

typedef struct xtDatamsgBasebandAP_header
{
  uint32_t frameCtr;
  uint32_t numOfBins;
  float binLength;
  float samplingFrequency;
  uint32_t carrierFrequency;
  float rangeOffset;
  // Amplitude buffer, numOfBins length.
  // Phase buffer, nomOfBins length.
} xtDatamsgBasebandAP_header_t;

// Setting default argument for receive data
int receive_data(bool print_data = false);




/*********************
 * 
 * SETUP FUNCTION
 * 
 ********************/

void setup (void)
{
    
  // Setup USB serial for debug communication
  SerialUSB.begin(921600);
  while (!SerialUSB);
  
  // Setup serial connection to radar (Serial1 is RX/TX pins on Arduino Zero)
  Serial1.begin(115200);
  
  // Change baudrate of radar module 
  SerialUSB.println("Changing baudrate of module to 921600...");
  setBaudRate(921600);

  // Change baudrate of Arduino's serial communication with radar
  Serial1.flush ();    // wait for send buffer to empty
  delay (100);         // let last characters be sent
  Serial1.end ();      // close serial

  // Restart serial with new baud rate
  Serial1.begin(921600);
  delay(2000);
  
  // Reset module
  empty_serial_buffer();
  SerialUSB.println("Resetting module...");
  send_command(&xts_spc_mod_reset, 1);

  // Receive system status messages
  SerialUSB.println("Waiting for system status messages...");
  while (!radar_ready()) {
    SerialUSB.println("Radar not ready, trying another reset...");
    empty_serial_buffer();
    send_command(&xts_spc_mod_reset, 1);
  }
  
  // Load module profile - Sleep (provides 2m radar frame length)
  SerialUSB.println("Loading sleep app...");
  load_sleep_app();

  // Turn on baseband (raw data) output.
  SerialUSB.println("Turning on raw data...");
  enable_raw_data();

  // Set detection zone (0.4 - 2.0 gives radar frame 0.3 - 2.3)
  SerialUSB.println("Setting detection zone...");
  setDetectionZone(0.4, 2.0);
  
  // Set high sensitivity. 
  SerialUSB.println("Setting sensitivity...");
  setSensitivity(9);

  // Start the app
  SerialUSB.println("Starting the app...");  
  execute_app();

  SerialUSB.println("Waiting for baseband data...");
}




/*********************
 * 
 * MAIN LOOP
 * 
 ********************/

void loop (void)
{
  // If the arduino is not fast enough to receive all the data from the radar, 
  // we need to empty the buffer of old data, to avoid lagging and overflow
  empty_serial_buffer();

  // Get data
  int len = receive_data();   // receive_data() fills recv_buf with a data package

  // Process data
  if (len > 0) {
    handleProtocolPacket(recv_buf+1, len); //recv_buf+1 to skip the start byte
  }
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
unsigned char serial_read_blocking()
{
  while (Serial1.available() < 1) {
    delay(1);
  }
  return Serial1.read();
}


/** 
 * Checks if the RX buffer is overflowing.
 * The Arduino RX buffer is only 64 bytes,
 * so this happens a lot with fast data rates
 */
bool check_overflow() 
{
  if (Serial1.available() >= SERIAL_BUFFER_SIZE-1) {
    return true;
  }

  return false;
}


// Empties the Serial RX buffer
void empty_serial_buffer()
{
  while (Serial1.available() > 0)
    Serial1.read();  // Remove one byte from the buffer
}


/**
 * Sends a command 
 */
void send_command(const unsigned char * cmd, int len) {

  // Calculate CRC
  char crc = xt_start;
  for (int i = 0; i < len; i++)
    crc ^= cmd[i];

  
  // Add escape bytes if necessary
  for (int i = 0; i < len; i++) {
    if (cmd[i] == 0x7D || cmd[i] == 0x7E || cmd[i] == 0x7F)
    {
     //TODO: Implement escaping 
     SerialUSB.write("CRC Escaping needed for send:buf! Halting...");
     while(1) {}
    }
  }
  
  // Send xt_start + command + crc_string + xt_stop
  Serial1.write(xt_start);
  Serial1.write(cmd, len);
  Serial1.write(crc);
  Serial1.write(xt_stop);
  Serial1.flush();
}


/**
 * Receives one data package from Serial buffer
 * Returns the length of the data received.
 */
int receive_data(bool print_data) 
{  
  int last_char = 0;
  int recv_len = 0;  //Number of bytes received
  unsigned char cur_char;
  
  //Wait for start character
  while (1) 
  {
    // Check if buffer is overflowed
    if (check_overflow())
      empty_serial_buffer();

    // Get one byte from radar
    cur_char = serial_read_blocking();
      
    if (cur_char == xt_escape)
    {
      // Check if buffer is overflowed
      if (check_overflow()) {
        return -1;
      }      
   
      // If it's an escape character –
      // ...ignore next character in buffer
      serial_read_blocking();
    }
    else if (cur_char == xt_start) 
    {
      // If it's the start character –  
      // ...we fill the first character of the buffer and move on
      recv_buf[0] = xt_start;
      recv_len = 1;
      break;
    }
  } 

  // Start receiving the rest of the bytes
  while (1) 
  {
    // Check if buffer is overflowed
    if (check_overflow()) {
      return -1;
    }
    
    // read a byte
    cur_char = serial_read_blocking();  // Get one byte from radar
  
    if (cur_char == xt_escape)
    {
      // Check if buffer is overflowed
      if (check_overflow()) {
        return -1;
      }
        
      // If it's an escape character –
      // fetch the next byte from serial
      cur_char = serial_read_blocking();
      
      // Make sure to not overwrite receive buffer
      if (recv_len >= RX_BUF_LENGTH)
        return -1;
  
      // Fill response buffer, and increase counter
      recv_buf[recv_len] = cur_char;
      recv_len++;
    }
    
    else if (cur_char == xt_start) 
    {
      // If it's the start character, something is wrong 
      return -1;
    }
    
    else 
    {
      // Make sure not overwrite receive buffer
      if (recv_len >= RX_BUF_LENGTH)
        break;
  
      // Fill response buffer, and increase counter
      recv_buf[recv_len] = cur_char;
      recv_len++;
      
      // is it the stop byte?
      if (cur_char == xt_stop) {
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
    crc ^= recv_buf[i];
    escape_found = 0;
  }


  // Print the received data
  if (print_data) {
    SerialUSB.print("Data: ");
    for (int i = 0; i < recv_len; i++) {
      SerialUSB.print(recv_buf[i] ,HEX);
      SerialUSB.print(" ");
    }
    SerialUSB.println(" ");
  }

  
  // Check if calculated CRC matches the recieved
  if (crc == recv_buf[recv_len-2])  {
    return recv_len;  // Return length of data packet upon success
  }
  else {
    //SerialUSB.println("[Error]: CRC check failed!");
    return -1; // Return -1 upon crc failure
  }

}



/*********************
 * 
 * PROTOCOL COMMANDS
 * 
 *********************/

 
/** 
 * Check if the packet is an acknowledge packet 
 */
void check_ack() 
{
  if (recv_buf[1] == xts_spr_system) {
    SerialUSB.println("Last received was XTS_SPR_SYSTEM message, trying new receive...");
    receive_data(true);
  }
  
  if (recv_buf[1] != xts_spr_ack) {
    SerialUSB.println("ACK not received! Halting...");
    while (1) {}
  }  
}



/**
 * Waiting for radar to become ready in the bootup sequence
 */
bool radar_ready() 
{
  // Try receiving xts_sprs_ready signal up to 5 times
  for (int i = 0; i < 5; i++) {
    receive_data(true);
    if (recv_buf[2] == xts_sprs_ready) {
      return true;
    }
    delay(500);
  }

  return false;
}


/**
 * Execute application
 */
void execute_app() 
{
  //Fill send buffer
  send_buf[0] = xts_spc_mod_setmode;
  send_buf[1] = xts_sm_run;

  // Send the command
  send_command(send_buf, 2);
  
  // Get response
  receive_data(true);
}


/**
 * Load sleep app
 */
void load_sleep_app() 
{
  //Fill send buffer
  send_buf[0] = xts_spc_mod_loadapp;
  memcpy(send_buf+1, &xts_id_app_sleep, 4);
  
  //Send the command
  send_command(send_buf, 5);
  
  //Get response
  receive_data(true);
  
  // Check if acknowledge was received
  check_ack();
  
}


/**
 * Enable Raw Data
 */
void enable_raw_data() 
{
  long data_length = 1;
  
  //Fill send buffer  
  send_buf[0] = xts_spc_dir_command;
  send_buf[1] = xts_sdc_app_setint;

  memcpy(send_buf+2, &xts_sacr_outputbaseband, 4);
  memcpy(send_buf+6, &data_length, 4);
  memcpy(send_buf+10, &xts_sacr_id_baseband_output_amplitude_phase, 4);

  //Send the command
  send_command(send_buf, 14);
  
  //Get response
  receive_data(true);

  // Check if acknowledge was received
  check_ack();
}


/**
 * Set the detection zone of the radar
 */
void setDetectionZone(float start_zone, float end_zone)
{
  send_buf[0] = xts_spc_appcommand;
  send_buf[1] = xts_spca_set;
  
  memcpy(send_buf+2, &xts_id_detection_zone, 4);
  memcpy(send_buf+6, &start_zone, 4);
  memcpy(send_buf+10, &end_zone, 4);
    
  //Send the command
  send_command(send_buf, 14);
  
  //Get response
  receive_data(true);

  // Check if acknowledge was received
  check_ack();
}


/**
 * Set sensitivity
 */
void setSensitivity(long sensitivity)
{
  send_buf[0] = xts_spc_appcommand;
  send_buf[1] = xts_spca_set;

  memcpy(send_buf+2, &xts_id_sensitivity, 4);
  memcpy(send_buf+6, &sensitivity, 4); 

  //Send the command
  send_command(send_buf, 10);
  
  //Get response
  receive_data(true);

  // Check if acknowledge was received
  check_ack();
  
}


/**
 * Change baud rate of radar serial interface
 */
void setBaudRate(uint32_t baudrate)
{
  //Fill send buffer  
  send_buf[0] = xts_spc_dir_command;
  send_buf[1] = xts_sdc_comm_setbaudrate;
  memcpy(send_buf+2, &baudrate, 4); 

  //Send the command
  send_command(send_buf, 6);
}





/************************
 * Higher-level commands
 ***********************/


/**
 * Processes the data packet
 */
void handleProtocolPacket(const unsigned char * data, unsigned int length)
{
  unsigned long pcontentId;
  float arrayAmplitude[52];
  
  if (data[0] == xts_spr_appdata)
  {
    // Get ID of the content
    memcpy(&pcontentId, data+1, 4);

    //Check ID of content
    if (pcontentId == xts_id_resp_status)
    {
      //SerialUSB.println("Respiration status data");
      // Todo: Could process complete respiration message here.
    }
    else if (pcontentId == xts_id_sleep_status)
    {
      //SerialUSB.println("Sleep status data");
      // Todo: Could process complete sleep message here.
    }
    else if (pcontentId == xts_id_baseband_amplitude_phase) 
    {
      //SerialUSB.println("Baseband Amplitude Data");
      xtDatamsgBasebandAP_header_t basebandHeader;
      memcpy(&basebandHeader, data+5, sizeof(xtDatamsgBasebandAP_header_t));
      
      if (basebandHeader.numOfBins != 52) {
        SerialUSB.print("[Error]: numOfBins = ");
        SerialUSB.println(basebandHeader.numOfBins);
      }
      
      memcpy(arrayAmplitude, data+5+sizeof(xtDatamsgBasebandAP_header_t), basebandHeader.numOfBins*sizeof(float));
      onDatamsgBasebandAP(&basebandHeader, arrayAmplitude);
    }
    else
    {
      //SerialUSB.println("Unknown data");
    }
  }
}


/**
 * Processes Baseband Amplitude data
 */
void onDatamsgBasebandAP( xtDatamsgBasebandAP_header_t *basebandHeader , float *arrayAmplitude)
{
  float triggerThreshold = 1.5;
  float adaptiveCluttermapWeight = 0.05;
  static float arrayCluttermap[BASEBAND_NUM_OF_BINS_MAX];
  float arrayAmplitudeFiltered[BASEBAND_NUM_OF_BINS_MAX];
    
  int index;
  float ampMax;
  float distance;
  static float distanceFiltered;
  float distanceFilterWeight = 0.3;
  //static uint32_t detectionTimeout = 0;
  static long last_detection = 0;

  // Check for strange values
  for (int i = 0; i < basebandHeader->numOfBins; i++) {
    if (abs(arrayAmplitude[i]) > 1000.0) {
      SerialUSB.println("Suspicious frame. Ignoring...");  
      return;
    }
  }
  

  // Adaptive cluttermap filtering
  for(int i=0; i<basebandHeader->numOfBins; i++)
  {
    arrayCluttermap[i] = arrayCluttermap[i]*(1-adaptiveCluttermapWeight) + arrayAmplitude[i]*adaptiveCluttermapWeight;
    arrayAmplitudeFiltered[i] = abs(arrayAmplitude[i] - arrayCluttermap[i]);
  }

  // Get index of max value
  index = get_max(arrayAmplitudeFiltered, basebandHeader->numOfBins, &ampMax);
  
  
  // Simple threshold, two different values based on distance.
  if (index>basebandHeader->numOfBins/2)
    triggerThreshold = 1.5;
  else 
    triggerThreshold = 3;

  // Detection
  if ((ampMax > triggerThreshold) && (ampMax < 1000)) 
  {
    last_detection = basebandHeader->frameCtr;
    
    distance = basebandHeader->rangeOffset + index * basebandHeader->binLength;
    distanceFiltered = distanceFiltered*(1-distanceFilterWeight) + distance*distanceFilterWeight;
    SerialUSB.print("Detection at ");
    SerialUSB.print((int)distanceFiltered);
    SerialUSB.print(".");
    SerialUSB.println((int)( (distanceFiltered - (int)distanceFiltered)*10));
    // SerialUSB.print(" amplitude: ");
    // SerialUSB.println(ampMax);
  }
  //else if (detectionTimeout++ > 20*5) // 5 seconds
  else if (basebandHeader->frameCtr - last_detection > 100) // 5 seconds
  {
    SerialUSB.print("No detection. ampMax: ");
    SerialUSB.println(ampMax);
  }
}


/**
 * Finds the highest amplitude in float array.
 * Returns position of highest amplitude.
 */
int get_max(float * data, int length, float * ampMax)
{
  float amax = 0.0;
  int idx = 0;
  
  for (int i = 0; i < length; i++) {
    if (data[i] > amax) {
      amax = data[i];
      idx = i;
    }
  }

  // Fill the incoming float point with the max amplitude 
  *ampMax =  amax;
  
  return idx;
}



/**
 * Function for printing a float array
 * Used for debugging.
 */
void print_data(float * data, int length)
{
  SerialUSB.println("Data:");

  for (int i = 0; i < length; i++) {
    SerialUSB.print(data[i]);
    SerialUSB.print(", ");
  }

  SerialUSB.println(" ");
}


