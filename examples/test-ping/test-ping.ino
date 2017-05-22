// Written by Øyvind Nydal Dahl
// www.build-electronic-circuits.com
// May 2017



#include <XeThruRadar.h>

XeThruRadar radar;


void setup (void)
{
  // Setup debug port
  radar.enableDebug();

  // Setup radar
  radar.init();
}



void loop (void)
{
  delay (3000);  // 3 seconds delay
  
  // Ping the radar
  radar.ping_radar();

  int mvm = radar.get_resp_movement();

}  




