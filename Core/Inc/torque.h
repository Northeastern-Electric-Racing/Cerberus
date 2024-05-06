/**
 * @file torque.h
 * @author Scott Abramson
 * @brief Convert pedal acceleration to torque and queue message with calcualted torque
 * @version 1.69
 * @date 2023-11-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TORQUE_H
#define TORQUE_H

#include "cmsis_os.h"

#define PIT_MAX_SPEED	 5.0 /* mph */
#define ACCUMULATOR_SIZE 10 /* size of the accumulator for averaging */

void vCalcTorque(void *pv_params);

/* Increases the torque limit by 10% */
void increase_torque_limit();

/* Decreases the torque limit by 10% */
void decrease_torque_limit();

extern osThreadId_t torque_calc_handle;
extern const osThreadAttr_t torque_calc_attributes;

#endif
