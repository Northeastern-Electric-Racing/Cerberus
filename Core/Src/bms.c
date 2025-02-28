#include "bms.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "cerberus_conf.h"
#include "fault.h"

osTimerId bms_timer;

static void bms_fault_callback(void *args)
{
	fault_data_t fault_data = { .id = BMS_CAN_MONITOR_FAULT,
				    .severity = DEFCON1 };
	fault_data.diag = "Failing To Receive CAN Messages from Shepherd";
	osTimerStart(bms_timer, BMS_CAN_MONITOR_DELAY);
	queue_fault(&fault_data);
}

void init_bms() {
	bms_timer = osTimerNew(bms_fault_callback, osTimerOnce, NULL, NULL);
}


void handle_dcl_msg()
{
	osTimerStart(bms_timer, BMS_CAN_MONITOR_DELAY);
}