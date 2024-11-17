#include "fault.h"
#include "task.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "state_machine.h"
#include "can_handler.h"
#include <string.h>
#include "c_utils.h"
#include "cerb_utils.h"

#define FAULT_HANDLE_QUEUE_SIZE 16
#define NEW_FAULT_FLAG		1U
#define NUM_OF_FAULTS		18UL

osMessageQueueId_t fault_handle_queue;

u_int32_t faults = 0;
fault_sev_t total_severity_level = DEFCON0;

osTimerId_t *timers = malloc(sizeof(osTimerId_t) * NUM_OF_FAULTS);

osStatus_t queue_fault(fault_data_t *fault_data)
{
	if (!fault_handle_queue)
		return -1;

	return queue_and_set_flag(fault_handle_queue, fault_data, fault_handle,
				  NEW_FAULT_FLAG);
}

osThreadId_t fault_handle;
const osThreadAttr_t fault_handle_attributes = {
	.name = "FaultHandler",
	.stack_size = 32 * 16,
	.priority = (osPriority_t)osPriorityRealtime7,
};

void vFaultHandler(void *pv_params)
{
	fault_data_t fault_data;
	fault_handle_queue = osMessageQueueNew(FAULT_HANDLE_QUEUE_SIZE,
					       sizeof(fault_data_t), NULL);

	for (;;) {
		osThreadFlagsWait(NEW_FAULT_FLAG, osFlagsWaitAny,
				  osWaitForever);

		while (osMessageQueueGet(fault_handle_queue, &fault_data, NULL,
					 osWaitForever) == osOK) {
			// Set Fault
			u_int32_t *fault_id = malloc((sizeof u_int32_t));
			*fault_id = (u_int32_t)fault_data.id;
			faults |= *fault_id;

			// Set Defcon
			uint8_t defcon = (uint8_t)fault_data.severity;
			if ((u_int16_t)total_severity_level <
			    (u_int16_t)defcon) {
				total_severity_level = defcon;
			}

			// Create Timers
			u_int32_t index = (u_int32_t)log2(*fault_id);

			if (timers[index] == NULL) {
				timers[index] = osTimerCreate(clearFault,
							      osTimerOnce,
							      fault_id, NULL);
			}

			if (osTimerStart(timers[index], 4000) != osOK) {
				return;
			}

			can_msg_t msg;
			msg.id = CANID_FAULT_MSG;
			msg.len = 8;

			memcpy(msg.data, &faults, sizeof(faults));
			memcpy(msg.data + sizeof(faults), &defcon,
			       sizeof(defcon));

			queue_can_msg(msg);
			printf("\r\nFault Handler! Diagnostic Info:\t%s\r\n\r\n",
			       fault_data.diag);

			switch (fault_data.severity) {
			case DEFCON1: /* Highest(1st) Priority */
				fault();
				break;
			case DEFCON2:
				fault();
				break;
			case DEFCON3:
				fault();
				break;
			case DEFCON4:
				break;
			case DEFCON5: /* Lowest Priority */
				break;
			default:
				break;
			}
		}
	}
}

void clearFault(void *args)
{
	u_int32_t *fault_num = (u_int32_t *)args;
	faults &= ~(*fault_num);
	free(fault_num);
}