#include "bms.h"
#include "timer.h"
#include "fault.h"
#include "can.h"
#include "serial_monitor.h"
#include "cerberus_conf.h"
#include <assert.h>
#include <stdlib.h>
#include "stdio.h"

#define CAN_QUEUE_SIZE 5

osMessageQueueId_t bms_monitor_queue;

bms_t *bms;

void bms_fault_callback();

osThreadId_t bms_monitor_handle;
const osThreadAttr_t bms_monitor_attributes = {
	.name = "BMSCANMonitor",
	.stack_size = 64 * 8,
	.priority = (osPriority_t)osPriorityHigh2
};

void bms_fault_callback()
{
	fault_data_t fault_data = {
		.id = BMS_CAN_MONITOR_FAULT, .severity = DEFCON1
	}; /*TO-DO: update severity*/
	fault_data.diag = "Failing To Receive CAN Messages from Shepherd";
	osTimerStart(bms->bms_monitor_timer, BMS_CAN_MONITOR_DELAY);
	queue_fault(&fault_data);
}

void bms_init()
{
	bms = malloc(sizeof(bms_t));
	assert(bms);

	/*TO-DO: specify timer attributes*/
	bms->bms_monitor_timer =
		osTimerNew(&bms_fault_callback, osTimerOnce, NULL, NULL);

	bms_monitor_queue =
		osMessageQueueNew(CAN_QUEUE_SIZE, sizeof(can_msg_t), NULL);
	assert(bms_monitor_queue);
}

void vAMSCANMonitor(void *pv_params)
{
	can_msg_t msg_from_queue;

	osTimerStart(bms->bms_monitor_timer, BMS_CAN_MONITOR_DELAY);
	for (;;) {
		osThreadFlagsWait(NEW_AMS_MSG_FLAG, osFlagsWaitAny,
				  osWaitForever);
		osMessageQueueGet(bms_monitor_queue, &msg_from_queue, NULL,
				  osWaitForever);
		osTimerStart(bms->bms_monitor_timer, BMS_CAN_MONITOR_DELAY);
	}
}
