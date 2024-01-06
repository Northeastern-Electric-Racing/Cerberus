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

void vCalcTorque(void* pv_params);
extern osThreadId_t torque_calc_handle;
extern const osThreadAttr_t torque_calc_attributes;

#endif
