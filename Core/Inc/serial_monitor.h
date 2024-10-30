#ifndef SERIAL_MONITOR_H
#define SERIAL_MONITOR_H

#include "cmsis_os.h"

/* Task for printing values to USART output */
void vSerialMonitor(void *pv_params);
extern osThreadId_t serial_monitor_handle;
extern const osThreadAttr_t serial_monitor_attributes;

#endif // SERIAL_MONITOR_H
