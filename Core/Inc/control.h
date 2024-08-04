/**
 * @file control.h
 * @author Scott Abramson
 * @brief Tasks for controlling the car.
 * @version 0.1
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef CONTROL_H
#define CONTROL_H

#define BRAKE_STATE_UPDATE_FLAG 0x00000001U

/**
 * @brief Task for controlling the brakelight.
 */
void vBrakelightControl(void *pv_params);
extern osThreadId brakelight_control_thread;
extern const osThreadAttr_t brakelight_monitor_attributes;

#endif
