/**
 * @file cerb_utils.c
 * @author Scott Abramson
 * @brief Utility functions for Cerberus.
 * @version 0.1
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "cerb_utils.h"

void debounce(bool input, osTimerAttr_t *timer, uint32_t period)
{
	if (input && !osTimerIsRunning(timer)) {
		/* Start timer if a button has been pressed */
		osTimerStart(timer, period);
	} else if (!input && osTimerIsRunning(timer)) {
		/* Button stopped being pressed. Kill timer. */
		osTimerStop(timer);
	}
}