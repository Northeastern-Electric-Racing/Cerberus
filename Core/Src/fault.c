#include "fault.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cerberus_conf.h"
#include "state_machine.h"

#define FAULT_HANDLE_QUEUE_SIZE 16
#define SEND_FAULT_TIME		500 /* in millis */

osMessageQueueId_t fault_handle_queue;
uint16_t crit_fault;
uint16_t non_crit_fault;
osTimerId_t *timers = NULL;

osStatus_t queue_fault(fault_data_t *fault_data)
{
	if (!fault_handle_queue)
		return osErrorParameter;

	osStatus_t status;

	if (fault_data->severity == CRITICAL) {
		status = osMessageQueuePut(fault_handle_queue, fault_data, 2,
					   0U);
	} else {
		status = osMessageQueuePut(fault_handle_queue, fault_data, 1,
					   0U);
	}

	return status;
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
		timers = calloc((MAX_CRITICAL_FAULT + MAX_NON_CRITICAL_FAULT),
				sizeof(osTimerId_t));
	}

	fault_data_t fault_data;
	fault_handle_queue = osMessageQueueNew(FAULT_HANDLE_QUEUE_SIZE,
					       sizeof(fault_data_t), NULL);
	for (;;) {
		if (osMessageQueueGet(fault_handle_queue, &fault_data, NULL,
				      pdMS_TO_TICKS(SEND_FAULT_TIME)) == osOK) {
			process_fault(fault_data);
		}

		// Send Can Message (even if a new fault was not received)
		can_msg_t msg;
		msg.id = CANID_FAULT_MSG;
		msg.len = 8;

		memcpy(msg.data, &(crit_fault), sizeof(crit_fault));
		memcpy(msg.data + sizeof(crit_fault), &non_crit_fault,
		       sizeof(non_crit_fault));

		queue_can_msg(msg);
	}
}

void process_fault(fault_data_t fault_data)
{
	// Set Fault
	uint32_t index = 0;
	uint32_t fault_id = 0;

	if (fault_data.severity == CRITICAL) {
		fault_id = (uint32_t)(1 << fault_data.fault_index.crit_fault);
		crit_fault |= fault_id;
		index = fault_data.fault_index.crit_fault;
		fault();
	} else if (fault_data.severity == NONCRITICAL) {
		fault_id =
			(uint32_t)(1 << fault_data.fault_index.non_crit_fault);
		non_crit_fault |= fault_id;
		index = fault_data.fault_index.non_crit_fault +
			MAX_CRITICAL_FAULT;
	}

	// Create Timers
	if (!timers[index]) {
		timers[index] =
			osTimerNew(clear_fault, osTimerOnce, &fault_data, NULL);
	}

	if (osTimerStart(timers[index], 4000) != osOK) {
		return;
	}

	printf("Fault Handler! Diagnostic Info:\t%s\n", fault_data.diag);
}

void clear_fault(void *args)
{
	fault_data_t *fault_data = (fault_data_t *)args;
	uint32_t fault_id = 0;

	if (fault_data->severity == CRITICAL) {
		fault_id = (uint32_t)(1 << fault_data->fault_index.crit_fault);
		crit_fault &= ~fault_id;
	} else if (fault_data->severity == NONCRITICAL) {
		fault_id =
			(uint32_t)(1 << fault_data->fault_index.non_crit_fault);
		non_crit_fault &= ~fault_id;
	}

	// unfault car if all critical faults are cleared
	if (crit_fault == 0) {
		set_ready_mode();
	}
}
