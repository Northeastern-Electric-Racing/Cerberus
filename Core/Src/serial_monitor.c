#include "serial_monitor.h"
#include <stdarg.h>
#include <stdio.h>

#define PRINTF_QUEUE_SIZE 16  /* Strings */
#define PRINTF_BUFFER_LEN 128 /* Characters */

osMessageQueueId_t printf_queue;
osThreadId_t serial_monitor_handle;
const osThreadAttr_t serial_monitor_attributes;

/*
 * Referenced https://github.com/esp8266/Arduino/blob/master/cores/esp8266/Print.cpp
 * Preformat string then put into a buffer
 */
int serial_print(const char* format, ...)
{
	va_list arg;
	char temp[PRINTF_BUFFER_LEN];
	size_t len;
	char* buffer = temp;

	/* Format Variadic Args into string */
	va_start(arg, format);
	len = vsnprintf(temp, sizeof(temp), format, arg);
	va_end(arg);

	/* Check to make sure we don't overflow buffer */
	if (len > sizeof(temp) - 1)
		return -1;

	osMessageQueuePut(printf_queue, &buffer, 0U, 0U);
	return 0;
}

void vSerialMonitor(void* pv_params)
{
	char* message;
	osStatus_t status;

	while(1){
		printf("TEST\n");
	}

	printf_queue = osMessageQueueNew(PRINTF_QUEUE_SIZE, sizeof(char*), NULL);

	for (;;) {
		/* Wait until new printf message comes into queue */
		status = osMessageQueueGet(printf_queue, &message, NULL, 0U);
		if (status != osOK) {
			// TODO: Trigger fault ?
		} else {
			printf(message);
		}
	}
}
