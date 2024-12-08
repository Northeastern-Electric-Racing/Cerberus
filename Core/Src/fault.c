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
#include "state_machine.h"

#define FAULT_HANDLE_QUEUE_SIZE 16
#define NEW_FAULT_FLAG		1U
#define NUM_OF_FAULTS		18UL

osMessageQueueId_t fault_handle_queue;

uint32_t faults = 0;

osTimerId_t *timers = NULL;

fault_sev_t max_severity_level = DEFCON_NONE;
fault_sev_t *severity_levels = NULL;

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
	.stack_size = 64 * 16,
	.priority = (osPriority_t)osPriorityRealtime7,
};

void vFaultHandler(void *pv_params)
{
	// Create Timers Array
	if (timers == NULL) {
		timers = calloc(NUM_OF_FAULTS, sizeof(osTimerId_t));
	}

	// Create Severity Levels Array (all DEFCON_NONE initially)
	if (severity_levels == NULL) {
		severity_levels = calloc(NUM_OF_FAULTS, sizeof(fault_sev_t));
		for (int i = 0; i < NUM_OF_FAULTS; i++) {
			severity_levels[i] = DEFCON_NONE;
		}
	}

	fault_data_t fault_data;
	fault_handle_queue = osMessageQueueNew(FAULT_HANDLE_QUEUE_SIZE,
					       sizeof(fault_data_t), NULL);

	for (;;) {
		osThreadFlagsWait(NEW_FAULT_FLAG, osFlagsWaitAny,
				  osWaitForever);

		while (osMessageQueueGet(fault_handle_queue, &fault_data, NULL,
					 osWaitForever) == osOK) {
			// Set Fault
			uint32_t *fault_id = malloc(sizeof(uint32_t));
			*fault_id = (uint32_t)fault_data.id;
			faults |= *fault_id;

			uint32_t index = (uint32_t)log2(*fault_id);

			// Create Timers
			if (!timers[index]) {
				timers[index] = osTimerNew(clearFault,
							   osTimerOnce,
							   fault_id, NULL);
			}

			if (osTimerStart(timers[index], 4000) != osOK) {
				return;
			}

			// Get New Maximum Severity Level
			severity_levels[index] = fault_data.severity;
			max_severity_level = getMaxSeverity();

			// Send Can Message
			can_msg_t msg;
			msg.id = CANID_FAULT_MSG;
			msg.len = 8;

			memcpy(msg.data, &faults, sizeof(faults));
			memcpy(msg.data + sizeof(faults), &max_severity_level,
			       sizeof(max_severity_level));

			queue_can_msg(msg);
			printf("Fault Handler! Diagnostic Info:\t%s\n",
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
			case DEFCON_NONE:
				break;
			default:
				break;
			}
		}
	}
}

void clearFault(void *args)
{
	uint32_t *fault_id = (uint32_t *)args;

	// Remove this timer's fault from total faults
	faults &= ~(*fault_id);

	// Remove this timer's severity from total severity
	severity_levels[(uint32_t)log2(*fault_id)] = DEFCON_NONE;
	max_severity_level = getMaxSeverity();

	// unfault car if all critical faults are cleared
	if (max_severity_level > DEFCON3) {
		set_ready_mode();
	}

	free(fault_id);
}

fault_sev_t getMaxSeverity()
{
	int max = DEFCON_NONE;
	for (int i = 0; i < NUM_OF_FAULTS; i++) {
		if (severity_levels[i] < max) {
			max = severity_levels[i];
		}
	}
	return (fault_sev_t)max;
}