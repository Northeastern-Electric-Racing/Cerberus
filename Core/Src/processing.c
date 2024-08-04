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
#include "state_machine.h"
#include "queues.h"
#include "cerb_utils.h"

#define TSMS_DEBOUNCE_PERIOD 500 /* ms */

static bool tsms = false;

bool get_tsms()
{
	return tsms;
}

void tsms_debounce_cb(void *arg)
{
	/* Set TSMS state to new debounced value */
	tsms = *((bool *)arg);
}

osThreadId_t process_tsms_thread_id;
const osThreadAttr_t process_tsms_attributes = {
	.name = "ProcessTSMS",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityHigh,
};

void vProcessTSMS(void *pv_params)
{
	bool tsms_reading;

	/* When the callback is called, the TSMS state will be set to the new reading in tsms_reading */
	osTimerId_t tsms_debounce_timer =
		osTimerNew(&tsms_debounce_cb, osTimerOnce, &tsms_reading, NULL);

	for (;;) {
		osThreadFlagsWait(TSMS_UPDATE_FLAG, osFlagsWaitAny,
				  osWaitForever);

		/* Get raw TSMS reading */
		osMessageQueueGet(tsms_data_queue, &tsms_reading, 0U,
				  osWaitForever);

		/* Debounce tsms reading */

		if (tsms_reading)
			debounce(tsms_reading, tsms_debounce_timer,
				 TSMS_DEBOUNCE_PERIOD);
		else
			/* Since debounce only debounces logic high signals, the reading must be inverted if it is low. Think of this as debouncing a "TSMS off is active" debounce. */
			debounce(!tsms_reading, tsms_debounce_timer,
				 TSMS_DEBOUNCE_PERIOD);

		if (get_func_state() == ACTIVE && tsms == false) {
			//TODO: make task notification to check if car should shut off if TSMS goes off, however this works for now
			set_home_mode();
		}
	}
}
