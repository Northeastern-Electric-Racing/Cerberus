/**
 * @file processing.c
 * @author Scott Abramson
 * @brief Definitions for tasks for processing data.
 * @version 0.1
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "processing.h"
#include "timer.h"
#include "state_machine.h"
#include "queues.h"

#define TSMS_DEBOUNCE_PERIOD 500 /* ms */

static bool tsms = false;

bool get_tsms()
{
	return tsms;
}

osThreadId_t process_tsms_thread_id;
const osThreadAttr_t process_tsms_attributes = {
	.name = "ProcessTSMS",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityHigh,
};

void vProcessTSMS(void *pv_params)
{
	nertimer_t tsms_debounce_timer = { .active = false };

	for (;;) {
		osThreadFlagsWait(TSMS_UPDATE_FLAG, osFlagsWaitAny,
				  osWaitForever);

		/* Get raw TSMS reading */
		bool tsms_reading;
		osMessageQueueGet(tsms_data_queue, &tsms_reading, 0U,
				  osWaitForever);

		/* Debounce tsms reading */

		// Timer has not been started, and there is a change in TSMS, so start the timer
		if (tsms != tsms_reading &&
		    !is_timer_active(&tsms_debounce_timer)) {
			start_timer(&tsms_debounce_timer, TSMS_DEBOUNCE_PERIOD);
		}
		// During debouncing, the tsms reading changes, so end the debounce period
		else if (tsms == tsms_reading &&
			 !is_timer_expired(&tsms_debounce_timer)) {
			cancel_timer(&tsms_debounce_timer);
		}
		// The TSMS reading has been consistent thorughout the debounce period
		else if (is_timer_expired(&tsms_debounce_timer)) {
			tsms = tsms_reading;
		}
		if (get_func_state() == ACTIVE && tsms == false) {
			//TODO: make task notification to check if car should shut off if TSMS goes off, however this works for now
			set_home_mode();
		}
	}
}

