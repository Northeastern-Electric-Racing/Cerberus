#include "serial_monitor.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINTF_QUEUE_SIZE 25  /* Strings */
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
	char* buffer = malloc(sizeof(char) * PRINTF_BUFFER_LEN);
	if (buffer == NULL)
		return -1;

	/* Format Variadic Args into string */
	va_start(arg, format);
	size_t len = vsnprintf(buffer, PRINTF_BUFFER_LEN, format, arg);
	va_end(arg);

	/* Check to make sure we don't overflow buffer */
	if (len > PRINTF_BUFFER_LEN - 1) {
		free(buffer);
		return -2;
	}

	/* If string can't be queued */
	osStatus_t stat = osMessageQueuePut(printf_queue, &buffer, 0U, 0U);
	if (stat) {
		free(buffer);
		return -3;
	}

	return 0;
}

void vSerialMonitor(void* pv_params)
{
	char* message;
	osStatus_t status;

	printf_queue = osMessageQueueNew(PRINTF_QUEUE_SIZE, sizeof(char*), NULL);

	for (;;) {
		/* Wait until new printf message comes into queue */
		status = osMessageQueueGet(printf_queue, &message, NULL, osWaitForever);
		if (status != osOK) {
			// TODO: Trigger fault ?
		} else {
			printf(message);
			free(message);
		}
	}
}
