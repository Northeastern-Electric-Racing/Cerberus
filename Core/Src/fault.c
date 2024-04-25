#include "fault.h"
#include "serial_monitor.h"
#include "task.h"
#include <assert.h>
#include "state_machine.h"
#include "timer.h"

#define FAULT_HANDLE_QUEUE_SIZE 16

osMessageQueueId_t fault_handle_queue;

osThreadId_t fault_handle;
const osThreadAttr_t fault_handle_attributes = {
	.name		= "FaultHandler",
	.stack_size = 32 * 8,
	.priority	= (osPriority_t)osPriorityRealtime1,
};

int queue_fault(fault_data_t* fault_data)
{
	if (!fault_handle_queue)
		return -1;
	can_msg_t msg = { .id = 0x069, .len = 8, .data = { 0 } };
	osMessageQueuePut(fault_handle_queue, fault_data, 0U, 0U);
	msg_data = ((fault_data.id << 8) | fault_data.severity) << 24;
	memcpy(msg.data, &msg_data, msg.len);
	queue_can_msg(msg);
	return 0;
}

void vFaultHandler(void* pv_params)
{
	fault_data_t fault_data;
	osStatus_t status;
	const state_req_t fault_request = {.id = FUNCTIONAL, .state.functional = FAULTED};
	fault_handle_queue = osMessageQueueNew(FAULT_HANDLE_QUEUE_SIZE, sizeof(fault_data_t), NULL);

	nertimer_t defcon2_timer;
	nertimer_t defcon2_reset;

	nertimer_t defcon3_timer;
	nertimer_t defcon3_reset;

	nertimer_t defcon4_timer;
	nertimer_t defcon4_reset;

	nertimer_t defcon5_timer;
	nertimer_t defcon5_reset;

	fault_handle_queue = osMessageQueueNew(FAULT_HANDLE_QUEUE_SIZE, sizeof(fault_data_t), NULL);
	
	for (;;) {
		/* Wait until a message is in the queue, send messages when they are in the queue */
		status = osMessageQueueGet(fault_handle_queue, &fault_data, NULL, osWaitForever);
		if (status == osOK) 
		{
			serial_print("\r\nFault Handler! Diagnostic Info:\t%s\r\n\r\n", fault_data.diag);
			switch (fault_data.severity) 
			{
			case DEFCON1: /* Highest(1st) Priority */
				serial_print("DEFCON 1 RECEIVED ENTER FAULTED");
				assert(osOK == queue_state_transition(fault_request));				
				break;
			case DEFCON2: /*Critical Warnings*/
				serial_print("DEFCON 2 RECEIVED");
				if(!is_timer_active(&defcon2_reset))
				{
					start_timer(&defcon2_reset, 1000);
				}
				else 
				{
					cancel_timer(&defcon2_reset);
					start_timer(&defcon2_reset, 1000);
				}
				if(!is_timer_active(&defcon2_timer))
				{
					if(is_timer_expired(&defcon2_timer))
					{
						serial_print("DEFCON 2 TIMER EXPIRED ENTER FAULTED");
						assert(osOK == queue_state_transition(fault_request));
					}
					else
					{
						start_timer(&defcon2_timer, 5000);
					}
				}
				break;
			case DEFCON3: /*Warnings*/
				serial_print("DEFCON 3 RECEIVED");
				if(!is_timer_active(&defcon3_reset))
				{
					start_timer(&defcon3_reset, 1000);
				}
				else 
				{
					cancel_timer(&defcon3_reset);
					start_timer(&defcon3_reset, 1000);
				}
				if(!is_timer_active(&defcon3_timer))
				{
					if(is_timer_expired(&defcon3_timer))
					{
						serial_print("DEFCON 3 TIMER EXPIRED ENTER FAULTED");
						assert(osOK == queue_state_transition(fault_request));
					}
					else
					{
						start_timer(&defcon3_timer, 15000);
					}
				}
			case DEFCON4: /*Lower priority warnings*/
				serial_print("DEFCON 4 RECEIVED");
				if(!is_timer_active(&defcon4_reset))
				{
					start_timer(&defcon4_reset, 1000);
				}
				else 
				{
					cancel_timer(&defcon4_reset);
					start_timer(&defcon4_reset, 1000);
				}
				if(!is_timer_active(&defcon4_timer))
				{
					if(is_timer_expired(&defcon4_timer))
					{
						serial_print("DEFCON 4 TIMER EXPIRED ENTER FAULTED");
						assert(osOK == queue_state_transition(fault_request));
					}
					else
					{
						start_timer(&defcon4_timer, 60000);
					}
				}
				break;
			case DEFCON5: /*Suggestions*/
				serial_print("DEFCON 5 RECEIVED");
				if(!is_timer_active(&defcon5_reset))
				{
					start_timer(&defcon5_reset, 1000);
				}
				else 
				{
					cancel_timer(&defcon5_reset);
					start_timer(&defcon5_reset, 1000);
				}
				if(!is_timer_active(&defcon5_timer))
				{
					if(is_timer_expired(&defcon5_timer))
					{
						serial_print("DEFCON 5 TIMER EXPIRED ENTER FAULTED");
						assert(osOK == queue_state_transition(fault_request));
					}
					else
					{
						start_timer(&defcon3_timer, 180000);
					}
				}
				break;
			default:
				serial_print("ALL CLEAR: NO FAULTS \n \r");
				break;
			}
			if(is_timer_expired(&defcon2_reset))
			{
				cancel_timer(&defcon2_timer);
			}
			if(is_timer_expired(&defcon3_reset))
			{
				cancel_timer(&defcon3_timer);
			}
			if(is_timer_expired(&defcon4_reset))
			{
				cancel_timer(&defcon4_timer);
			}
			if(is_timer_expired(&defcon5_reset))
			{
				cancel_timer(&defcon5_timer);
			}
		}
	}
}