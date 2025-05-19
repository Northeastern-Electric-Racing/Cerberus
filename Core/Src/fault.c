#include "fault.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "can_handler.h"
#include "cerberus_conf.h"
#include "state_machine.h"

#define FAULT_HANDLE_QUEUE_SIZE 16
#define SEND_FAULT_TIME		500 /* in millis */

osMessageQueueId_t fault_handle_queue;
uint16_t crit_fault;
uint16_t non_crit_fault;
osTimerId_t *timers = NULL;

/**
 * @brief callback function to clear fault after timeout
 * 
 * @param args fault header with index and severity
 */
static void clear_fault(void *args)
{
	uint32_t *fault_id = (uint32_t *)args;
	uint32_t fault_index;

	if (*fault_id < MAX_CRITICAL_FAULT) {
		fault_index = (1 << *fault_id);
		crit_fault &= ~fault_index;
	} else if (*fault_id > MAX_CRITICAL_FAULT &&
		   *fault_id < MAX_NON_CRITICAL_FAULT) {
		fault_index = (1 << (*fault_id - MAX_CRITICAL_FAULT - 1));
		non_crit_fault &= ~fault_index;
	}

	// unfault car if all critical faults are cleared, we are faulted, and the current fault is critical
	if (crit_fault == 0 && get_func_state() == FAULTED &&
	    *fault_id < MAX_CRITICAL_FAULT) {
		set_ready_mode();
	}

	free(fault_id);
}

/**
 * @brief adds incoming fault data to current fault state
 * 
 * @param fault_data includes diag, index, and severity
 */
static void process_fault(fault_data_t fault_data)
{
	// Set Fault
	uint32_t index = 0;

	if (fault_data.fault_id < MAX_CRITICAL_FAULT) {
		index = fault_data.fault_id;
		crit_fault |= (uint32_t)(1 << index);
		fault();
	} else if (fault_data.fault_id > MAX_CRITICAL_FAULT &&
		   fault_data.fault_id < MAX_NON_CRITICAL_FAULT) {
		index = fault_data.fault_id - MAX_CRITICAL_FAULT - 1;
		non_crit_fault |= (uint32_t)(1 << index);
	}

	// Create Timers
	if (!timers[index]) {
		uint32_t *fault_id = malloc(sizeof(uint32_t));
		assert(fault_id);

		*fault_id = fault_data.fault_id;

		timers[index] =
			osTimerNew(clear_fault, osTimerOnce, fault_id, NULL);
	}

	if (osTimerStart(timers[index], 4000) != osOK) {
		return;
	}

	printf("Fault Handler! Diagnostic Info:\t%s\n", fault_data.diag);
}

osStatus_t queue_fault(fault_data_t *fault_data)
{
	if (!fault_handle_queue)
		return osErrorParameter;

	osStatus_t status;

	if (fault_data->fault_id < MAX_CRITICAL_FAULT) {
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
		timers = calloc((MAX_NON_CRITICAL_FAULT - 1),
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
		can_msg_t msg = { .id = CANID_FAULT_MSG,
				  .len = 8,
				  .data = { 0 } };

		uint8_t crit_bytes[2];
		uint8_t noncrit_bytes[2];

		crit_bytes[0] = crit_fault;
		crit_bytes[1] = crit_fault >> 8;
		noncrit_bytes[0] = non_crit_fault;
		noncrit_bytes[1] = non_crit_fault >> 8;

		crit_bytes[0] = reverse_bits(crit_bytes[0]);
		crit_bytes[1] = reverse_bits(crit_bytes[1]);
		noncrit_bytes[0] = reverse_bits(noncrit_bytes[0]);
		noncrit_bytes[1] = reverse_bits(noncrit_bytes[1]);

		memcpy(msg.data, crit_bytes, sizeof(crit_bytes));
		memcpy(msg.data + sizeof(crit_bytes), noncrit_bytes,
		       sizeof(noncrit_bytes));

		queue_can_msg(msg);
	}
}
