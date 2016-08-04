//LED pins
const int red_pin = 3;
const int green_pin = 6;
const int blue_pin = 5;


// Radar constants
const unsigned char xt_start = 0x7D;
const unsigned char xt_stop = 0x7E;
const unsigned char xt_escape = 0x7F;

const unsigned char xts_spc_appcommand = 0x10;
const unsigned char xts_spc_mod_setmode = 0x20;
const unsigned char xts_spc_mod_loadapp = 0x21;
const unsigned char xts_spc_mod_reset = 0x22;
const unsigned char xts_spc_mod_setledcontrol = 0x24;
const unsigned char xts_spr_appdata = 0x50;

const long xts_id_app_resp = 0x1423a2d6;
const long xts_id_resp_status = 0x2375fe26;

const unsigned char xts_sm_run = 0x01;
const unsigned char xts_sm_normal = 0x10;
const unsigned char xts_sm_idle = 0x11;

const unsigned char xts_val_resp_state_breathing = 0; 		// Valid RPM detected Current RPM value
const unsigned char xts_val_resp_state_movement = 1; 		// Detects motion, but can not identify breath 0
const unsigned char xts_val_resp_state_movement_tracking = 2;	// Detects motion, possible breathing 0
const unsigned char xts_val_resp_state_no_movement = 3; 	// No movement detected 0
const unsigned char xts_val_resp_state_initializing = 4; 	// No movement detected 0
const unsigned char xts_val_resp_state_unknown = 6;


unsigned char recv_buf[64]; // Buffer for receiving data from radar. Size picked at random


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
    }
  }
  
  // Send xt_start + command + crc_string + xt_stop
  Serial.write(xt_start);
  Serial.write(cmd, len);
  Serial.write(crc);
  Serial.write(xt_stop);
  
}
  
  
  
int receive_data() {
  
  // Get response

  char last_char = 0x00;
  int recv_len = 0;  //Number of bytes received

  //Wait for start character
  while (1) 
  {
    char c = Serial.read();	// Get one byte from radar
    
    if (c == xt_escape)
    {
      // If it's an escape character –
      // ...ignore next character in buffer
      Serial.read();
    }
    else if (c == xt_start) 
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
    // read a byte
    char cur_char = Serial.read();	// Get one byte from radar
    
    if (cur_char == -1) {
      continue;
    }
      
    // Fill response buffer, and increase counter
    recv_buf[recv_len] = cur_char;
    recv_len++;
    
    // is it the stop byte?
    if (cur_char == xt_stop) {
      //setColor(255, 0, 0, 1.0);
      if (last_char != xt_escape)
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
    if (recv_buf[i] == xt_escape && !escape_found) {
      escape_found = 1;
      continue;
    }
    else {
      crc ^= recv_buf[i];
      escape_found = 0;
    }
  }
  
  
  // Check if calculated CRC matches the recieved
  if (crc == recv_buf[recv_len-2]) 
  {
    return 0;  // Return 0 upon success
  }
  else 
  {
    return -1; // Return -1 upon crc failure
  } 
}


void empty_serial_buffer()
{
  while (Serial.available())
  {
    Serial.read();	// Remove one byte from the buffer
  } 
}


void get_respiration_data() {
  
  //The arduino is not fast enough to receive all the data from the radar (it seems)
  //...so always empty the buffer of old data, so that we are sure we have fresh data
  empty_serial_buffer();
  
  if (receive_data() != 0)
  {
     //TODO: Something went wrong! 
     //blink_green();
     //setColor(255, 0, 0, 1.0);
     return;
  }
  
  // Now recv_buf should be filled with valid data
  
  
  // Check that it's app-data we've received
  if (recv_buf[1] != xts_spr_appdata)
  {
    //TODO: Something went wrong!
    //blink_red();
    return;
  }
  
  
  // Check that xts_id_resp_status is correct (just to make sure we are getting the right data)
  long xirs_recv = recv_buf[5] | (recv_buf[4] << 8) | (recv_buf[3] << 16) | (recv_buf[2] << 24);
  if (xirs_recv != xts_id_resp_status) 
  {
    //TODO: Something went wrong!
    //blink_red();
    //return;
  }
  

  float * movement;
  float brightness = 0;
  
  // State code
  char state_code = recv_buf[10];
  
  switch (state_code) {
    case xts_val_resp_state_breathing:

      //"Breathing detected:"

      //Point float pointer to the first byte of the float in the buffer
      movement = (float*)&recv_buf[22];      
      
      //Get the float value
      brightness = *movement;
      
      //Movement is usually between -1 and 1, so move it to 0 to 1 instead:
      brightness += 1.0;
      brightness = brightness/2.0;
      
      //Cut off values under 0 and over 1
      if (brightness < 0.0)
        setColor(0, 0, 255, 0.0);
      else if (brightness > 1.0)
        setColor(0, 0, 255, 1.0);
      else  
        setColor(0, 0, 255, brightness);
        
      break;
    case xts_val_resp_state_movement:
      // "Movement"
      setColor(255, 255, 0, 1.0);
      break;
    case xts_val_resp_state_movement_tracking:
      // "Movement tracking"
      setColor(0, 255, 0, 1.0);
      break;
    case xts_val_resp_state_no_movement:
      // "No movement"
      setColor(255, 255, 255, 1.0);
      break;
    case xts_val_resp_state_initializing:
      // "Initializing"
      setColor(255, 0, 255, 1.0);
      break;
    case xts_val_resp_state_unknown:
      // "State unknown"
      setColor(255, 0, 0, 1.0);
      break;
    default:  
      setColor(255, 0, 0, 1.0);
      break;
  }
 
}

// RESET MODULE
void reset_module() {
  send_command(&xts_spc_mod_reset, 1);
  if (receive_data()) 	// Receives the "booting" state
    blink_blue();
  if (receive_data()) 	// Receives the "ready" state
    blink_blue();
}

// Load respiration app
void load_respiration_app() 
{
  //Fill send buffer
  unsigned char send_buf[5];
  send_buf[0] = xts_spc_mod_loadapp;
  send_buf[4] = (xts_id_app_resp >> 24) & 0xff;
  send_buf[3] = (xts_id_app_resp >> 16) & 0xff;  
  send_buf[2] = (xts_id_app_resp >> 8) & 0xff;
  send_buf[1] = xts_id_app_resp & 0xff;
  
  //Send the command
  send_command(send_buf, 5);
  if (receive_data()) 	// Receives the acknowledge
    blink_blue();  
}

// Execute application
void execute_app() 
{
  //Fill send buffer
  unsigned char send_buf[2];
  send_buf[0] = xts_spc_mod_setmode;
  send_buf[1] = xts_sm_run;

  //Send the command
  send_command(send_buf, 2);
  if (receive_data()) 	// Receives the acknowledge
    blink_blue();  
}


void setColor(int red, int green, int blue, float brightness)
{
  analogWrite(red_pin, 255 - red * brightness);
  analogWrite(green_pin, 255 - green * brightness);
  analogWrite(blue_pin, 255 - blue * brightness);
}
 
void blink_red() {
  while(1) {
    setColor(255, 0, 0, 1.0);
    delay(500);
    setColor(0, 0, 0, 0.0);
    delay(500);
  }
}

void blink_green() {
  while(1) {
    setColor(0, 255, 0, 1.0);
    delay(500);
    setColor(0, 0, 0, 0.0);
    delay(500);
  }
}

void blink_blue() {
  while(1) {
    setColor(0, 0, 255, 1.0);
    delay(500);
    setColor(0, 0, 0, 0.0);
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);
  pinMode(red_pin, OUTPUT);


  //Test RGB-led
  setColor(255, 0, 0, 1.0);
  delay(500);
  setColor(0, 0, 0, 0.0);
  delay(500);

  setColor(0, 255, 0, 1.0);
  delay(500);
  setColor(0, 0, 0, 0.0);
  delay(500);

  setColor(0, 0, 255, 1.0);
  delay(500);
  setColor(0, 0, 0, 0.0);
  delay(500);
  
  
  reset_module();
  load_respiration_app();
  execute_app();

  //Set initializing color
  setColor(255, 0, 255, 1.0);
  
}


void loop() {
  get_respiration_data();
}
