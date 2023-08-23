/**
 * @file cascadiamc.c
 * @author Hamza Iqbal
 * @brief Source file for Motor Controller Driver
 * @version 0.1
 * @date 2023-08-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "cascadiamc.h"

void MC_init(mc_t* mc)
{

}

void MC_update(mc_t* mc)
{

}

int32_t MC_get_rpm(mc_t* mc)
{

}

int16_t MC_get_duty_cycle(mc_t* mc)
{

}

int16_t MC_get_input_voltage(mc_t* mc)
{

}

int16_t MC_get_ac_current(mc_t* mc)
{

}

int16_t MC_get_dc_current(mc_t* mc)
{

}

int16_t MC_get_controller_temp(mc_t* mc)
{

}

int16_t MC_get_motor_temp(mc_t* mc)
{

}

uint8_t MC_get_fault_code(mc_t* mc)
{

}

int8_t MC_get_throttle_signal(mc_t* mc)
{

}

int8_t MC_get_brake_signal(mc_t* mc)
{

}

int8_t MC_get_drive_enable(mc_t* mc)
{

}

void MC_set_brake_current(int16_t brake_current)                    /* SCALE: 10          UNITS: Amps       */
{

}                   

void MC_set_current(int16_t current)                                /* SCALE: 10          UNITS: Amps       */
{

}                               

void MC_set_speed(int32_t rpm)                                      /* SCALE: 1           UNITS: RPM        */
{

}                                     

void MC_set_position(int16_t angle)                                 /* SCALE: 10          UNITS: Degrees    */
{

}                                

void MC_set_relative_current(int16_t relative_current)              /* SCALE: 10          UNITS: Percentage */
{

}             

void MC_set_relative_brake_current(int16_t relative_brake_current)  /* SCALE: 10          UNITS: Percentage */
{

} 

void MC_set_digital_output(uint8_t output, bool value)              /* SCALE: 1           UNITS: No units   */
{

}            

void MC_set_max_ac_current(int16_t current)                         /* SCALE: 10          UNITS: Amps       */
{

} 

void MC_set_max_ac_brake_current(int16_t current)                   /* SCALE: 10          UNITS: Amps       */
{

}                  

void MC_set_max_dc_current(int16_t current)                         /* SCALE: 10          UNITS: Amps       */
{

}                        

void MC_set_max_dc_brake_current(int16_t current)                   /* SCALE: 10          UNITS: Amps       */
{

}                  

void MC_set_drive_enable(bool drive_enable)                         /* SCALE: 1           UNITS: No units   */
{

}                        