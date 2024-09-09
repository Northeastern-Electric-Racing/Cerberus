#include "serial_monitor.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cerb_utils.h"

#define PRINTF_QUEUE_SIZE 25 /* Strings */
#define PRINTF_BUFFER_LEN 128 /* Characters */
#define NEW_MESSAGE_FLAG  0x00000001U

osMessageQueueId_t printf_queue;
osThreadId_t serial_monitor_handle;
const osThreadAttr_t serial_monitor_attributes = {
	.name = "SerialMonitor",
	.stack_size = 32 * 32,
	.priority = (osPriority_t)osPriorityHigh,
};

/*
 * Referenced https://github.com/esp8266/Arduino/blob/master/cores/esp8266/Print.cpp
 * Preformat string then put into a buffer
 */
int serial_print(const char *format, ...)
{
	va_list arg;
	char *buffer = malloc(sizeof(char) * PRINTF_BUFFER_LEN);
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

	osStatus_t stat = queue_and_set_flag(
		printf_queue, &buffer, serial_monitor_handle, NEW_MESSAGE_FLAG);
	if (stat) {
		free(buffer);
		return -3;
	}

	return 0;
}

void vSerialMonitor(void *pv_params)
{
	char *message;

	printf_queue =
		osMessageQueueNew(PRINTF_QUEUE_SIZE, sizeof(char *), NULL);

	for (;;) {
		osThreadFlagsWait(NEW_MESSAGE_FLAG, osFlagsWaitAny,
				  osWaitForever);

		/* Get message to print */
		while (osMessageQueueGet(printf_queue, &message, NULL, 0U) ==
		       osOK) {
			printf(message);
			free(message);
		}
	}
}
