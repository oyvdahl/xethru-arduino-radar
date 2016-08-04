#ifndef XeThruRadar_h
#define XeThruRadar_h

#include "Arduino.h"
  


class XeThruRadar
{
  public:
    XeThruRadar();
    
    void init();
    int get_resp_state();
    int get_rpm();
    float get_resp_movement();
    int get_raw();
    
    void reset_module();
    void load_respiration_app();
    void load_sleep_app();
    void execute_app();
    void enable_raw_data();
    
  private:
    void send_command(const unsigned char * cmd, int len);
    int receive_data();
    int receive_raw_data();
    void empty_serial_buffer();
    
    // Radar constants
    const unsigned char _xt_start = 0x7D;
    const unsigned char _xt_stop = 0x7E;
    const unsigned char _xt_escape = 0x7F;

    const unsigned char _xts_spc_appcommand = 0x10;
    const unsigned char _xts_spc_mod_setmode = 0x20;
    const unsigned char _xts_spc_mod_loadapp = 0x21;
    const unsigned char _xts_spc_mod_reset = 0x22;
    const unsigned char _xts_spc_mod_setledcontrol = 0x24;
    const unsigned char _xts_spr_appdata = 0x50;

    // Values taken from example code.
    const unsigned char _xts_spc_dir_command = 0x90;            // Not found in serial protocol document as of 8th of July 2016. /Oyvind Dahl
    const unsigned char _xts_sdc_app_setint = 0x71;             // Not found in serial protocol document as of 8th of July 2016. /Oyvind Dahl
    const unsigned char _xts_sacr_outputbaseband = 0x10;        // Not found in serial protocol document as of 8th of July 2016. /Oyvind Dahl
    
    const unsigned char _xts_sacr_id_baseband_output_iq_amplitude_phase = 0x02;
    
    const long _xts_id_app_resp = 0x1423a2d6;
    const long _xts_id_app_sleep = 0x00f17b17;
    const long _xts_id_resp_status = 0x2375fe26;

    const unsigned char _xts_sm_run = 0x01;
    const unsigned char _xts_sm_normal = 0x10;
    const unsigned char _xts_sm_idle = 0x11;

    const unsigned char _xts_val_resp_state_breathing = 0; 		// Valid RPM detected Current RPM value
    const unsigned char _xts_val_resp_state_movement = 1; 		// Detects motion, but can not identify breath 0
    const unsigned char _xts_val_resp_state_movement_tracking = 2;	// Detects motion, possible breathing 0
    const unsigned char _xts_val_resp_state_no_movement = 3; 		// No movement detected 0
    const unsigned char _xts_val_resp_state_initializing = 4; 		// No movement detected 0
    const unsigned char _xts_val_resp_state_unknown = 6;

    unsigned char _recv_buf[256]; // Buffer for receiving data from radar. Size picked at random

};

#endif