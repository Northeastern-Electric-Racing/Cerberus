/**
 * @file control.c
 * @author Scott Abramson
 * @brief Tasks for controlling the car.
 * @version 0.1
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "pdu.h"
#include "queues.h"
#include "control.h"

osThreadId brakelight_control_thread;
const osThreadAttr_t brakelight_monitor_attributes = {
	.name = "BrakelightMonitor",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityHigh,
};

void vBrakelightControl(void *pv_params)
{
	pdu_t *pdu = (pdu_t *)pv_params;

	for (;;) {
		osThreadFlagsWait(BRAKE_STATE_UPDATE_FLAG, osFlagsWaitAny,
				  osWaitForever);
		write_brakelight(pdu, get_brake_state());
	}
}
