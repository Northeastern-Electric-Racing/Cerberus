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

#include "can_handler.h"
#include "cerberus_conf.h"
#include "dti.h"
#include "queues.h"
#include <assert.h>
#include "serial_monitor.h"

#define MAX_TORQUE 10.0 /* Nm */

// TODO: Might want to make these more dynamic to account for MechE tuning
#define MIN_PEDAL_VAL 0x1DC /* Raw ADC */
#define MAX_PEDAL_VAL 0x283 /* Raw ADC */

/* DO NOT ATTEMPT TO SEND TORQUE COMMANDS LOWER THAN THIS VALUE */
#define MIN_COMMAND_FREQ  60					  /* Hz */
#define MAX_COMMAND_DELAY 1000 / MIN_COMMAND_FREQ /* ms */

osThreadId_t torque_calc_handle;
const osThreadAttr_t torque_calc_attributes = { .name		= "SendTorque",
												.stack_size = 128 * 8,
												.priority = (osPriority_t)osPriorityAboveNormal5 };

void vCalcTorque(void* pv_params)
{
	const uint16_t delay_time = 5; /* ms */
	/* End application if we try to update motor at freq below this value */
	assert(delay_time < MAX_COMMAND_DELAY);
	pedals_t pedal_data;
	uint16_t torque = 0;
	osStatus_t stat;
	float accel = 0;

	// TODO: Get important data from MC
	// dti_t *mc = (dti_t *)pv_params;

	for (;;) {
		stat = osMessageQueueGet(pedal_data_queue, &pedal_data, 0U, delay_time);

		/* If we receive a new message within the time frame, calc new torque */
		if (stat == osOK)
		{
			// TODO: Add state based torque calculation

			accel = (float)pedal_data.accelerator_value;

			if (accel < MIN_PEDAL_VAL) {
				torque = 0;
			}

			else {
				/* Linear scale of torque */
				float torque_rate = (MAX_TORQUE / MAX_PEDAL_VAL);
				torque = torque_rate * (accel - MIN_PEDAL_VAL) * 4.0;

			}
		}
		else
		{
			serial_print("I'm scared.");
		}

		/* Send whatever torque command we have on record */
		dti_set_torque(torque);
	}
}
 