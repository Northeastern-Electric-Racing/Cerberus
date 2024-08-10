#ifndef NERO_H
#define NERO_H

#include <stdbool.h>
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

/*
 * Tells NERO the current MPH
*/
void set_mph(int8_t new_mph);

/**
 * @brief Send NERO information over CAN.
 * 
 */
void send_nero_msg();

#endif // NERO_H