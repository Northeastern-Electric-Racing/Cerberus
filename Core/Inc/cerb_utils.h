/**
 * @file cerb_utils.h
 * @author Scott Abramson
 * @brief Utility functions for Cerberus.
 * @version 0.1
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef CERB_UTILS_H
#define CERB_UTILS_H

#include "cmsis_os.h"
#include "stdbool.h"
#include "timer.h"

/**
 * @brief Function to debounce a signal. Debounce is started and maintained by a high signal, and it is interrupted by a low signal. The callback is called in a thread context.
 * 
 * @param input Input to debounce.
 * @param timer Timer for debouncing input.
 * @param period The period of time, in milliseconds, to debounce the input for.
 * @param cb Callback to be called if the input is successfully debounced.
 * @param arg Argument of the callback function.
 */
void debounce(bool input, nertimer_t *timer, uint32_t period,
	      void (*cb)(void *arg), void *arg);

/**
 * @brief Queue a message and set a thread noticication flag. The message is queued with a timeout of 0.
 * 
 * @param queue Queue to put message in.
 * @param msg_ptr Pointer to message to put in queue.
 * @param thread_id ID of thread that is being notified.
 * @param flags Flag to set in thread's notification array.
 * @return osStatus_t The error code of queueing the message.
 */
osStatus_t queue_and_set_flag(osMessageQueueId_t queue, const void *msg_ptr,
			      osThreadId_t thread_id, uint32_t flags);
#endif
