#include <stdarg.h>
#include "serial_monitor.h"

#define PRINTF_QUEUE_SIZE   16
osMessageQueueId_t printf_queue;

void serial_print(char* format, ...)
{
    char* msg;

    va_list arg_ptr;

    va_start(arg_ptr, void);

    // TODO: Loop through and format string accordingly
    // Is the a structure that can take in a variable number of args and datatypes in C?
    // snprintf(msg, 50, format, s);

    va_end(arg_ptr);

    osMessageQueuePut(printf_queue, &msg , 0U, 0U);
}

void vSerialMonitor(void *pv_params)
{
    char* message;
	osStatus_t status;

    // TODO: Initialize UART?
    
    printf_queue = osMessageQueueNew(PRINTF_QUEUE_SIZE, sizeof(char*), NULL);

    for (;;) {
        /* Wait until new printf message comes into queue */
		status = osMessageQueueGet(printf_queue, &message, NULL, 0U);
		if (status != osOK) {
			//TODO: Trigger fault ?
		} else {
            printf(*message);
        }
    }
}
