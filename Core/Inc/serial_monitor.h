#ifndef SERIAL_MONITOR_H
#define SERIAL_MONITOR_H

#include "cmsis_os.h"

/* Function to queue a message to be sent on the UART stream */
int serial_print(const char *format, ...);

/* Defining Temperature Monitor Task */
void vSerialMonitor(void *pv_params);

extern osThreadId_t serial_monitor_handle;
extern const osThreadAttr_t serial_monitor_attributes;

#endif // SERIAL_MONITOR_H
