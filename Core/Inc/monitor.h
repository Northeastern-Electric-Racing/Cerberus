#ifndef MONITOR_H
#define MONITOR_H

#include "cmsis_os.h"

/* Defining Temperature Monitor Task */
void vTempMonitor(void *pv_params);

osThreadId_t temp_monitor_handle;
const osThreadAttr_t temp_monitor_attributes = {
  .name = "TempMonitor",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal3,
};

#endif // MONITOR_H