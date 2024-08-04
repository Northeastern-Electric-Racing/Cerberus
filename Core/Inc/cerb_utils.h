/**
 * @file cerb_utils.h
 * @author Scott Abramson
 * @brief Utility functions for Cerberus.
 * @version 0.1
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef CERB_UTILS_H
#define CERB_UTILS_H

#include "cmsis_os.h"
#include "stdbool.h"

/**
 * @brief Function to debounce an input with an RTOS timer. Debounce is started and maintained by a high signal, and it is interrupted by a low signal.
 * 
 * @param input Input to debounce.
 * @param timer RTOS timer with callback to be called at end of debounce period.
 * @param period The period of time, in milliseconds, to debounce the input for.
 */
void debounce(bool input, osTimerAttr_t *timer, uint32_t period);

#endif
