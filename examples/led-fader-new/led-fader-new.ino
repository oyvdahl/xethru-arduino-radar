// Written by Øyvind Nydal Dahl
// www.build-electronic-circuits.com
// May 2017
//

#include <XeThruRadar.h>

// Serial port for debugging (Change to match the serial port on your Arduino)
#define SerialDebug SerialUSB


//LED pins
const int red_pin = 3;
const int green_pin = 6;
const int blue_pin = 5;

XeThruRadar radar;


// Help variables
int error_count = 0;

void setup() {
  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);

  //Set LED to pink(?) while initializing radar
  setColor(255, 0, 255, 1.0);
  
  // Setup debug port in library (for developers)
  // Note: Do not use without making sure the same port is NOT used in this sketch
  //radar.enableDebug();

  // Setup debug port for this sketch
  SerialDebug.begin(115200);

  // I use a delay so that I have 5 seconds to connect the radar after programming
  delay(5000);

  // Setup radar
  radar.init();
  
  // Tell the radar to load the respiration app
  radar.load_respiration_app();

  // Set detection zone (0.4 - 2.0 gives radar frame 0.3 - 2.3)
  radar.setDetectionZone(0.4, 2.0);

  // Set high sensitivity.
  radar.setSensitivity(5);

  // Start the app (the radar will start sending a constant stream of measurements)
  radar.execute_app();

}


void loop() {

  // Get respiration data
  RespirationData data = radar.get_respiration_data();

  if (data.valid_data == true) 
  {
    // Set brightness of LED if in breathing state
    if (data.state_code == radar._xts_val_resp_state_breathing) {
      //Movement is usually between -1 and 1, so move it to 0 to 1 instead:
      float brightness = data.movement + 1.0;
      brightness += 1.0;
      brightness = brightness/2.0;
  
      // Set brightness of the blue LED
      setColor(0, 0, 255, brightness);
  
      SerialDebug.println(data.movement);
    }
    else if (data.state_code == radar._xts_val_resp_state_movement) {
      setColor(0, 255, 255, 1.0); // Set color to yellow
      SerialDebug.println("Detects motion, but can not identify breath");
    }
    else if (data.state_code == radar._xts_val_resp_state_movement_tracking) {
      setColor(0, 255, 255, 1.0); // Set color to yellow
      SerialDebug.println("Detects motion, possible breathing");
    }
    else if (data.state_code == radar._xts_val_resp_state_no_movement) {
      setColor(0, 255, 255, 1.0); // Set color to yellow
      SerialDebug.println("No movement detected");
    }
    else if (data.state_code == radar._xts_val_resp_state_initializing) {
      setColor(0, 255, 255, 1.0); // Set color to yellow
      SerialDebug.println("State: Initializing");
    }
    else {
      setColor(0, 255, 255, 1.0); // Set color to yellow
      SerialDebug.println("Unknown state");
    }
  } 
  else {
    setColor(255, 0, 0, 1.0); // Set color to red
    SerialDebug.println("Valid respiration data NOT received!");
  }
}



void setColor(int red, int green, int blue, float brightness)
{
  //Make sure brightness is between 0 and 1:
  if (brightness > 1.0)
    brightness = 1.0;
  else if (brightness < 0.0)
    brightness = 0.0;
  
  //Set the brightness of each color
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
