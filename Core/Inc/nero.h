#ifndef NERO_H
#define NERO_H

#include <stdbool.h>
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

/**
 * @brief Record new MPH data and send it to NERO.
 * 
 * @param new_mph New MPH data
 */
void set_mph(int8_t new_mph);

/**
 * @brief Send NERO information over CAN.
 * 
 */
void send_nero_msg();

#endif // NERO_H