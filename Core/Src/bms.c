#include "bms.h"
#include "timer.h"
#include "fault.h"
#include "can.h"
#include "cerberus_conf.h"
#include <assert.h>
#include <stdlib.h>

#define BMS_CAN_MONITOR_DURATION 100 /*Duration of between petting bms monitor watchdog (in ticks)*/
#define CAN_QUEUE_SIZE 5

osMessageQueueId_t bms_monitor_queue;

void bms_fault_callback();

osThreadId_t bms_monitor_handle;
const osThreadAttr_t bms_monitor_attributes = {
	.name = "BMSCANMonitor",
	.stack_size = 64 * 8,
	.priority = (osPriority_t)osPriorityHigh2
};

void bms_fault_callback()
{
	fault_data_t fault_data = { .id = BMS_CAN_MONITOR_FAULT, .severity = DEFCON1 }; /*TO-DO: update severity*/

	fault_data.diag = "Failing To Receive CAN Messages from Shepherd";
	queue_fault(&fault_data);
}

bms_t* bms_init()
{

	bms_t* bms = malloc(sizeof(bms_t));
	assert(bms);

	/*TO-DO: specify timer attributes*/
	bms->bms_monitor_timer = osTimerNew(&bms_fault_callback, osTimerOnce, NULL, NULL);

	bms_monitor_queue = osMessageQueueNew(CAN_QUEUE_SIZE, sizeof(can_msg_t), NULL);
	assert(bms_monitor_queue);

	return bms;
}

void vBMSCANMonitor(void* pv_params)
{
    bms_t* bms = (bms_t*)pv_params;
	can_msg_t msg_from_queue;

	for (;;) {
		if (osOK == osMessageQueueGet(bms_monitor_queue, &msg_from_queue, NULL, osWaitForever)) {
			/*TO-DO: fix duration (ticks)*/
			osTimerStart(bms->bms_monitor_timer, BMS_CAN_MONITOR_DURATION);
		}
	}
}