/**
 * @file torque.c
 * @author Scott Abramson
 * @brief Convert pedal acceleration to torque and queue message with calcualted torque
 * @version 1.69
 * @date 2023-11-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <assert.h>
#include "can_handler.h"
#include "queues.h"
#include "cerberus_conf.h"
#include "dti.h"

#define MAX_TORQUE		10 /* Nm */

//TODO: Might want to make these more dynamic to account for MechE tuning
#define MIN_PEDAL_VAL	2 /* Raw ADC */
#define MAX_PEDAL_VAL	2 /* Raw ADC */

/* DO NOT ATTEMPT TO SEND TORQUE COMMANDS LOWER THAN THIS VALUE */
#define MIN_COMMAND_FREQ	60 /* Hz */
#define MAX_COMMAND_DELAY	1000 / MIN_COMMAND_FREQ /* ms */

osThreadId_t torque_calc_handle;
const osThreadAttr_t torque_calc_attributes = {
	.name = "SendTorque",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityAboveNormal4
};

void vCalcTorque(void* pv_params) {

	const uint16_t delay_time  = 5; /* ms */

	/* End application if we try to update motor at freq below this value */
	assert(delay_time < MAX_COMMAND_DELAY);

	pedals_t pedal_data;
	uint16_t torque;
	osStatus_t stat;
	dti_t mc;

	dti_init(&mc);

	for (;;) {
		stat = osMessageQueueGet(pedal_data_queue, &pedal_data, 0U, delay_time);

		/* If we receive a new message within the time frame, calc new torque */
		if (stat == osOK) {
			// TODO: Add state based torque calculation

			uint16_t accel = pedal_data.accelerator_value;
			
			if (accel < MIN_PEDAL_VAL)
				torque = 0;
			else
				/* Linear scale of torque */
				torque = (MAX_TORQUE / MAX_PEDAL_VAL) * accel - MIN_PEDAL_VAL;
		}

		/* Send whatever torque command we have on record */
		dti_set_torque(torque);
	}
}