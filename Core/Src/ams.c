#include "ams.h"
#include "timer.h"
#include "fault.h"
#include "can.h"
#include "serial_monitor.h"
#include "cerberus_conf.h"
#include <assert.h>
#include <stdlib.h>
#include "stdio.h"

ams_t *ams;

void ams_fault_callback()
{
	fault_data_t fault_data = { .id = AMS_CAN_MONITOR_FAULT,
				    .severity = DEFCON1 };
	fault_data.diag = "Failing To Receive CAN Messages from Shepherd";
	osTimerStart(ams->ams_monitor_timer, AMS_CAN_MONITOR_DELAY);
	queue_fault(&fault_data);
}

void ams_init()
{
	ams = malloc(sizeof(ams_t));
	assert(ams);

	ams->ams_monitor_timer =
		osTimerNew(&ams_fault_callback, osTimerOnce, NULL, NULL);
}

void handle_dcl_msg()
{
	osTimerStart(ams->ams_monitor_timer, AMS_CAN_MONITOR_DELAY);
}