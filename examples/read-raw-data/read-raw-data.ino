// Written by Øyvind Nydal Dahl
// www.build-electronic-circuits.com
// July 2016



#include <XeThruRadar.h>
#include <SPI.h>

XeThruRadar radar;


void setup (void)
{
  // Setup radar
  radar.init();

  // Reset module
  radar.reset_module();

  // Load module profile - Sleep (provides 2m radar frame length)
  radar.load_sleep_app();

  // Turn on baseband (raw data) output.
  radar.enable_raw_data();
  
  // Set detection zone (0.4 - 2.0 gives radar frame 0.3 - 2.3)
  
  // Set high sensitivity. 
  
  // Start module
  radar.execute_app();


  // Setup SPI communication
  digitalWrite(SS, HIGH);  // ensure SS stays high for now
  SPI.begin ();

  // Slow down the master a bit
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  
}



void loop (void)
{
  float radar_num = (float)radar.get_raw();
  
  //If it returns -99.0, that means it did not receive a proper measurement
  //if (movement == -99.0) {
  //  return;
  //}
  
  spi_send_data(radar_num);
  delay (1000);  // 1 seconds delay 
}  // end of loop



void spi_send_data(float numero) 
{
  char c;
  char charVal[7];

  dtostrf(numero, 4, 2, charVal);  //4 is mininum width, 4 is precision; float value is copied onto buff
  
  // enable Slave Select
  digitalWrite(SS, LOW);    // SS is pin 10
  
  // send test string
  //for (const char * p = "Hello, world!\n" ; c = *p; p++)
  //  SPI.transfer (c);
  for (int i=0; i<7; i++)
    SPI.transfer (charVal[i]);
  SPI.transfer ('\n');
  //SPI.transfer (c);
  
  // disable Slave Select
  digitalWrite(SS, HIGH);

}




