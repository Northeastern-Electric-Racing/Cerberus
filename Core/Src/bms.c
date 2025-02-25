#include "bms.h"
#include "cmsis_os2.h"

#include <assert.h>
#include <stdlib.h>

#include "cerberus_conf.h"
#include "fault.h"

static void bms_fault_callback(void* args)
{
	bms_t* bms = (bms_t*)args;
	if (osMutexAcquire(bms->mutex, osWaitForever) == osOK) {
		fault_data_t fault_data = { .id = BMS_CAN_MONITOR_FAULT, .severity = DEFCON1 };
		fault_data.diag			= "Failing To Receive CAN Messages from Shepherd";
		osTimerStart(bms->bms_monitor_timer, BMS_CAN_MONITOR_DELAY);
		queue_fault(&fault_data);
		osMutexRelease(bms->mutex);
	}
}

bms_t* bms_init()
{
	bms_t* bms = malloc(sizeof(bms_t));
	assert(bms);

	bms->bms_monitor_timer = osTimerNew(&bms_fault_callback, osTimerOnce, bms, NULL);
	bms->mutex = osMutexNew(NULL);

	return bms;
}

void handle_dcl_msg(bms_t* bms)
{
	if (osMutexAcquire(bms->mutex, osWaitForever) == osOK) {
		osTimerStart(bms->bms_monitor_timer, BMS_CAN_MONITOR_DELAY);
		osMutexRelease(bms->mutex);
	}
}