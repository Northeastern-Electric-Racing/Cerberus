#include "fault.h"
#include "c_utils.h"
#include "can_handler.h"
#include "serial_monitor.h"
#include "state_machine.h"
#include "task.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define FAULT_HANDLE_QUEUE_SIZE 16

osMessageQueueId_t fault_handle_queue;

osThreadId_t fault_handle;
const osThreadAttr_t fault_handle_attributes = {
	.name = "FaultHandler",
	.stack_size =

		52 * 8,
	.priority = (osPriority_t)osPriorityRealtime1,
};

int queue_fault(fault_data_t* fault_data)
{
	if (!fault_handle_queue)
		return -1;

	osMessageQueuePut(fault_handle_queue, fault_data, 0U, 0U);
	return 0;
}

void vFaultHandler(void* pv_params)
{
	fault_data_t fault_data;
	osStatus_t status;
	const state_req_t fault_request = { .id = FUNCTIONAL, .state.functional = FAULTED };
	fault_handle_queue = osMessageQueueNew(FAULT_HANDLE_QUEUE_SIZE, sizeof(fault_data_t), NULL);

	for (;;) {
		/* Wait until a message is in the queue, send messages when they are in the queue */
		status = osMessageQueueGet(fault_handle_queue, &fault_data, NULL, osWaitForever);
		if (status == osOK) {
			uint32_t fault_id = (uint32_t)fault_data.id;
			endian_swap(&fault_id, 4);
			uint8_t defcon = (uint8_t)fault_data.severity;

			can_msg_t msg;
			msg.id	= CANID_FAULT_MSG;
			msg.len = 8;
			uint8_t msg_data[8];
			memcpy(msg_data, &fault_id, 4);
			memcpy(msg_data + 4, &defcon, 1);
			memcpy(msg.data, msg_data, msg.len);
			queue_can_msg(msg);
			printf("\r\nFault Handler! Diagnostic Info:\t%s\r\n\r\n", fault_data.diag);
			switch (fault_data.severity) {
			case DEFCON1: /* Highest(1st) Priority */
				queue_state_transition(fault_request);
				break;
			case DEFCON2:
				queue_state_transition(fault_request);
				break;
			case DEFCON3:
				queue_state_transition(fault_request);
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