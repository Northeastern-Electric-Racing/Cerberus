#include "bms.h"

#include <assert.h>
#include <stdlib.h>

#include "cerberus_conf.h"
#include "fault.h"

bms_t *bms;

void bms_fault_callback()
{
	fault_data_t fault_data = { .id = BMS_CAN_MONITOR_FAULT,
				    .severity = DEFCON1 };
	fault_data.diag = "Failing To Receive CAN Messages from Shepherd";
	osTimerStart(bms->bms_monitor_timer, BMS_CAN_MONITOR_DELAY);
	queue_fault(&fault_data);
}

void bms_init()
{
	bms = malloc(sizeof(bms_t));
	assert(bms);

	bms->bms_monitor_timer =
		osTimerNew(&bms_fault_callback, osTimerOnce, NULL, NULL);
}

void handle_dcl_msg()
{
	osTimerStart(bms->bms_monitor_timer, BMS_CAN_MONITOR_DELAY);
}