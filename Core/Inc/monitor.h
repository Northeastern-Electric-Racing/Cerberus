#ifndef CERBERUS_MONITOR_H
#define CERBERUS_MONITOR_H

#include "cmsis_os.h"

/* Defining Temperature Monitor Task */
void vTempMonitor(void *pv_params);

extern osThreadId_t temp_monitor_handle;
extern const osThreadAttr_t temp_monitor_attributes;

#endif // MONITOR_H