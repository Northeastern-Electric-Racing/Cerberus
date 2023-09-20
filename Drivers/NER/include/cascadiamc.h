/**
 * @file cascadiamc.h
 * @author Hamza Iqbal
 * @brief Driver to abstract sending and receiving CAN messages to control cascadia motor controller
 * @version 0.1
 * @date 2023-08-09
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef CASCADIAMC_H
#define CASCADIAMC_H

#include <stdint.h>
#include <stdbool.h>   
#include "stm32f4xx_hal.h"
#include "can.h"

#define NUM_POLES
typedef struct
{
    int32_t rpm;            /* SCALE: 1         UNITS: Rotations per Minute   */
    int16_t duty_cycle;     /* SCALE: 10        UNITS: Percentage             */
    int16_t input_voltage;  /* SCALE: 1         UNITS: Volts                  */
    int16_t ac_current;     /* SCALE: 10        UNITS: Amps                   */
    int16_t dc_current;     /* SCALE: 10        UNITS: Amps                   */
    int16_t cont_temp;      /* SCALE: 10        UNITS: Degrees Celsius        */
    int16_t motor_temp;     /* SCALE: 10        UNITS: Degrees Celsius        */
    uint8_t fault_code;     /* SCALE: 1         UNITS: No units just a number */
    int8_t throttle_signal; /* SCALE: 1         UNITS: Percentage             */
    int8_t brake_signal;    /* SCALE: 1         UNITS: Percentage             */
    int8_t drive_enable;    /* SCALE: 1         UNITS: No units just a number */
} cascadiamc_t;

void cascadiamc_init(cascadiamc_t* mc);

/*
 * SCALE: bool
 * UNITS: No units      
 */
int8_t cascadiamc_get_drive_enable(cascadiamc_t* mc);

/*
 * SCALE: 10
 * UNITS: Amps       
 */
int8_t cascadiamc_set_brake_current(int16_t brake_current);                   

/*
 * SCALE: 10
 * UNITS: Amps       
 */
int8_t cascadiamc_set_current(int16_t current);

/*
 * SCALE: 1
 * UNITS: RPM       
 */
int8_t cascadiamc_set_speed(int32_t rpm);               

/*
 * SCALE: 10
 * UNITS: Degrees     
 */
int8_t cascadiamc_set_position(int16_t angle);

/*
 * SCALE: 10
 * UNITS: Percentage     
 */
int8_t cascadiamc_set_relative_current(int16_t relative_current);     

/*
 * SCALE: 10
 * UNITS: Percentage       
 */
int8_t cascadiamc_set_relative_brake_current(int16_t relative_brake_current); 

/*
 * SCALE: 1
 * UNITS: No units       
 */
int8_t cascadiamc_set_digital_output(uint8_t output, bool value);      

/*
 * SCALE: 10
 * UNITS: Amps       
 */
int8_t cascadiamc_set_max_ac_current(int16_t current);                   

/*
 * SCALE: 10
 * UNITS: Amps       
 */
int8_t cascadiamc_set_max_ac_brake_current(int16_t current);         

/*
 * SCALE: 10
 * UNITS: Amps       
 */
uint8_t cascadiamc_set_max_dc_current(int16_t current);                    

/*
 * SCALE: 10
 * UNITS: Amps       
 */
int8_t cascadiamc_set_max_dc_brake_current(int16_t current);                

/*
 * SCALE: bool
 * UNITS: No units      
 */
int8_t cascadiamc_set_drive_enable(bool drive_enable);

#endif