#include "bms.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cerberus_conf.h"
#include "fault.h"

osTimerId bms_timer;

static osMutexAttr_t bms_mutex_attributes;
static bms_t bms;

static void bms_fault_callback(void *args)
{
	fault_data_t fault_data = {
		.fault_id = BMS_CAN_MONITOR_FAULT,
	};
	fault_data.diag = "Failing To Receive CAN Messages from Shepherd";
	osTimerStart(bms_timer, BMS_CAN_MONITOR_DELAY);
	queue_fault(&fault_data);
}

void bms_init()
{
	bms.mutex = osMutexNew(&bms_mutex_attributes);
	assert(bms.mutex);

	bms.battbox_temp = 0;

	bms_timer = osTimerNew(bms_fault_callback, osTimerOnce, NULL, NULL);
	osTimerStart(bms_timer, BMS_CAN_MONITOR_DELAY);

	assert(&bms);
}

void handle_dcl_msg()
{
	osTimerStart(bms_timer, BMS_CAN_MONITOR_DELAY);
}

osStatus_t bms_get_battbox_temp(uint16_t *temp)
{
	osStatus_t stat = osMutexAcquire(bms.mutex, osWaitForever);
	if (stat)
		return stat;
	memcpy(temp, &bms.battbox_temp, sizeof(bms.battbox_temp));
	return osMutexRelease(bms.mutex);
}

void bms_record_battbox_temp(can_msg_t msg)
{
	osMutexAcquire(bms.mutex, osWaitForever);
	bms.battbox_temp =
		(msg.data[0] << 8) +
		msg.data[1]; // Get "BMS/Cells/Temp_High_Value" (first two bytes of the "Cell Temperatures" CAN message)
	osMutexRelease(bms.mutex);
}