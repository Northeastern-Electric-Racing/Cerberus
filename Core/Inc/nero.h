#ifndef NERO_H
#define NERO_H

#include <stdbool.h>
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#define NERO_UPDATE_FLAG 1U

/*
 * Tells NERO the current MPH
*/
void set_mph(int8_t new_mph);

/**
 * @brief Send NERO information over CAN.
 * 
 */
void send_mode_status();

/*
* Emits the state of NERO
*/
void vNeroMonitor(void *pv_params);
extern osThreadId_t nero_monitor_handle;
extern const osThreadAttr_t nero_monitor_attributes;

#endif // NERO_H