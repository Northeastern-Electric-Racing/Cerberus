/**
 * @file cerb_utils.c
 * @author Scott Abramson
 * @brief Utility functions for Cerberus.
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "cerb_utils.h"

void debounce(bool input, nertimer_t *timer, uint32_t period,
	      void (*cb)(void *arg), void *arg)
{
	if (input && !is_timer_active(timer)) {
		start_timer(timer, period);
	} else if (!input && is_timer_active(timer)) {
		/* Input is no longer high, so stop timer */
		cancel_timer(timer);
	} else if (input && is_timer_expired(timer)) {
		cb(arg);
	}
}

osStatus_t queue_and_set_flag(osMessageQueueId_t queue, const void *msg_ptr,
			      osThreadId_t thread_id, uint32_t flags)
{
	osStatus_t status = osMessageQueuePut(queue, msg_ptr, 0U, 0U);
	osThreadFlagsSet(thread_id, flags);
	return status;
}