#include "fault.h"
#include "serial_monitor.h"
#include "task.h"
#include <assert.h>
#include <stdio.h>
#include "state_machine.h"
#include "can_handler.h"
#include <string.h>
#include "c_utils.h"

#define FAULT_HANDLE_QUEUE_SIZE 16
#define NEW_FAULT_FLAG		1U

osMessageQueueId_t fault_handle_queue;

int queue_fault(fault_data_t *fault_data)
{
	if (!fault_handle_queue)
		return -1;

	osStatus_t error =
		osMessageQueuePut(fault_handle_queue, fault_data, 0U, 0U);
	osThreadFlagsSet(fault_handle, NEW_FAULT_FLAG);
	return error;
}

osThreadId_t fault_handle;
const osThreadAttr_t fault_handle_attributes = {
	.name = "FaultHandler",
	.stack_size = 52 * 8,
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

		osMessageQueueGet(fault_handle_queue, &fault_data, NULL,
				  osWaitForever);

		uint32_t fault_id = (uint32_t)fault_data.id;
		endian_swap(&fault_id, sizeof(fault_id));
		uint8_t defcon = (uint8_t)fault_data.severity;

		can_msg_t msg;
		msg.id = CANID_FAULT_MSG;
		msg.len = 8;
		uint8_t msg_data[8];
		memcpy(msg_data, &fault_id, sizeof(fault_id));
		memcpy(msg_data + sizeof(fault_id), &defcon, sizeof(defcon));
		memcpy(msg.data, msg_data, msg.len);
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