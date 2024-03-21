#include "bms_can_monitor.h"
#include "timer.h"
#include "fault.h"
#include "can.h"
#include <assert.h>
#include <stdlib.h>

#define BMS_WATCHDOG_DURATION 10 /*Duration of btween petting bms monitor watchdog*/
#define CAN_QUEUE_SIZE 5

osThreadId_t bms_can_monitor_handle;
const osThreadAttr_t bms_can_monitor_attributes = {
	.name = "BMSCANMonitor",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityLow1 /*Adjust priority*/
};

bms_can_monitor_t* bms_can_monitor_init() {

    bms_can_monitor_t* bms_can_monitor = malloc(sizeof(bms_can_monitor_t));
	assert(bms_can_monitor);

    bms_can_monitor_queue = osMessageQueueNew(CAN_QUEUE_SIZE, sizeof(can_msg_t), NULL);
    assert(bms_can_monitor_queue);

    return bms_can_monitor;
}

void vBMSCANMonitor(void* pv_params) 
{
	fault_data_t fault_data = { .id = BMS_CAN_MONITOR_FAULT, .severity = DEFCON1 }; /*Ask about severity*/
    
    bms_can_monitor_t* bms_can_monitor = (bms_can_monitor_t*)pv_params;
	can_msg_t msg_from_queue;

	for (;;) {
		if (osOK == osMessageQueueGet(bms_can_monitor_queue, &msg_from_queue, NULL, osWaitForever)) {
			if (!is_timer_active(&bms_can_monitor->timer)) {
				start_timer(&bms_can_monitor->timer, BMS_WATCHDOG_DURATION);
			} else {
				cancel_timer(&bms_can_monitor->timer);
			}
		}
		if (is_timer_expired(&bms_can_monitor->timer)) {
			fault_data.diag = "Failing To Receive CAN Messages from Shepherd";
			queue_fault(&fault_data);
		}
		osDelay(BMS_CAN_MONITOR_DELAY)
	}
}