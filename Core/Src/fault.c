#include "fault.h"
#include "serial_monitor.h"
#include "task.h"
#include <assert.h>
#include <stdio.h>
#include "state_machine.h"
#include "can_handler.h"
#include <string.h>
#include "c_utils.h"
#include "cerb_utils.h"

#define FAULT_HANDLE_QUEUE_SIZE 16
#define NEW_FAULT_FLAG		1U

osMessageQueueId_t fault_handle_queue;

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
			uint32_t fault_id = (uint32_t)fault_data.id;
			endian_swap(&fault_id, sizeof(fault_id));
			uint8_t defcon = (uint8_t)fault_data.severity;

			can_msg_t msg;
			msg.id = CANID_FAULT_MSG;
			msg.len = 8;

			memcpy(msg.data, &fault_id, sizeof(fault_id));
			memcpy(msg.data + sizeof(fault_id), &defcon,
			       sizeof(defcon));

			queue_can_msg(msg);
			serial_print(
				"\r\nFault Handler! Diagnostic Info:\t%s\r\n\r\n",
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