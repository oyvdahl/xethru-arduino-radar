#include <XeThruRadar.h>


//LED pins
const int red_pin = 3;
const int green_pin = 6;
const int blue_pin = 5;

XeThruRadar radar;


// Help variables
int error_count = 0;

void setup() {
  radar.init();
  
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);
  pinMode(red_pin, OUTPUT);
  
  radar.reset_module();
  radar.load_respiration_app();
  radar.execute_app();

  //Set initializing color
  setColor(255, 0, 255, 1.0);
}


void loop() {
  float movement = radar.get_resp_movement();
  
  //If it returns -99.0, that means it did not receive a proper measurement
  if (movement == -99.0) {
     error_count++;
     if (error_count > 5) {
       setColor(255, 0, 0, 1.0);
       
       //Reset error counter
       error_count = 0;
     }
  }
  else {
    //Reset error counter
    error_count = 0;
    
    //Movement is usually between -1 and 1, so move it to 0 to 1 instead:
    float brightness = movement + 1.0;
    brightness += 1.0;
    brightness = brightness/2.0;
    
    setColor(0, 0, 255, brightness);
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
