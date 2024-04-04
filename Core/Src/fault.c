#include "fault.h"
#include "serial_monitor.h"
#include "task.h"
#include <assert.h>
#include "state_machine.h"

#define FAULT_HANDLE_QUEUE_SIZE 16

osMessageQueueId_t fault_handle_queue;

osThreadId_t fault_handle;
const osThreadAttr_t fault_handle_attributes = {
	.name		= "FaultHandler",
	.stack_size = 128 * 8,
	.priority	= (osPriority_t)osPriorityHigh7,
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
	fault_handle_queue = osMessageQueueNew(FAULT_HANDLE_QUEUE_SIZE, sizeof(fault_data_t), NULL);
	uint8_t critical_warning_count = 0;
	uint8_t iteration_count = 0;
	for (;;) {
		/* Wait until a message is in the queue, send messages when they are in the queue */
		status = osMessageQueueGet(fault_handle_queue, &fault_data, NULL, osWaitForever);
		if (status == osOK) 
		{
			serial_print("\r\nFault Handler! Diagnostic Info:\t%s\r\n\r\n", fault_data.diag);
			switch (fault_data.severity) 
			{
			case DEFCON1: /* Highest(1st) Priority */
				serial_print("DEFCON 1: Enter Faulted");
				assert(osOK == queue_func_state(FAULTED));
				break;
			case DEFCON2: /*Critical Warnings*/
				critical_warning_count++;
				serial_print("Critical Warning Count: %s", critical_warning_count);
				if(critical_warning_count == 10 && iteration_count == 10)
				{
					assert(osOK == queue_func_state(FAULTED));
					critical_warning_count = 0;
					iteration_count = 0;
				}
				break;
			case DEFCON3: /*Warnings*/
				iteration_count = 0;
				break;
			case DEFCON4: /*Lower priority warnings*/
				iteration_count = 0;
				break;
			case DEFCON5: /*Suggestions*/
				iteration_count = 0;
				break;
			default:
				serial_print("ALL CLEAR: NO FAULTS \n \r");
				iteration_count = 0;
				break;
			}
			iteration_count++;
		}

		/* Yield to other tasks */
		osThreadYield();
	}
}