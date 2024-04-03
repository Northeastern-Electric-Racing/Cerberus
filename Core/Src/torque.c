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
#include "state_machine.h"

#define MAX_TORQUE 10.0 /* Nm */

// TODO: Might want to make these more dynamic to account for MechE tuning
#define MIN_PEDAL_VAL 0x1DC /* Raw ADC */
#define MAX_PEDAL_VAL 0x283 /* Raw ADC */

/* DO NOT ATTEMPT TO SEND TORQUE COMMANDS LOWER THAN THIS VALUE */
#define MIN_COMMAND_FREQ  60					  /* Hz */
#define MAX_COMMAND_DELAY 1000 / MIN_COMMAND_FREQ /* ms */

static uint16_t torque_accumulator[ACCUMULATOR_SIZE];

static float torque_limit_percentage = 1.0;

osThreadId_t torque_calc_handle;
const osThreadAttr_t torque_calc_attributes = { .name		= "SendTorque",
												.stack_size = 128 * 8,
												.priority = (osPriority_t)osPriorityAboveNormal5 };

static void linear_accel_to_torque(float accel, uint16_t* torque)
{
	/* Linearly map acceleration to torque */
	*torque = (uint16_t)((accel / MAX_TORQUE) * 0xFFFF);
}

static void rpm_to_mph(uint32_t rpm, float* mph)
{
	/* Convert RPM to MPH */
	*mph = (rpm / 60) * WHEEL_CIRCUMFERENCE * 2.237 / GEAR_RATIO;
}

static void limit_accel_to_torque(float accel, uint16_t* torque)
{
		float mph;
		rpm_to_mph(dti_get_rpm(), &mph);
		uint16_t newVal;
		//Results in a value from 0.5 to 0 (at least halving the max torque at all times in pit or reverse)
		if (mph > PIT_MAX_SPEED) {
			newVal = 0;
		}
		else {
			float torque_derating_factor = fabs(0.5 + ((-0.5/PIT_MAX_SPEED) * mph));
			newVal = accel * torque_derating_factor;
		}
		uint16_t ave = 0;
		uint16_t temp[ACCUMULATOR_SIZE];
		memcpy(torque_accumulator, ACCUMULATOR_SIZE, temp);
		for (int i = 0; i < ACCUMULATOR_SIZE - 1; i++) {
			temp[i + 1] = torque_accumulator[i];
			ave += torque_accumulator[i+1];
		}
		ave += newVal;
		ave /= ACCUMULATOR_SIZE;
		temp[0] = newVal;
		if(torque > ave) {
			torque = ave;
		}
		memcpy(temp, ACCUMULATOR_SIZE, torque_accumulator);
}

static void paddle_accel_to_torque(float accel, uint16_t* torque)
{
	*torque = (uint16_t)torque_limit_percentage * ((accel / MAX_TORQUE) * 0xFFFF);
	//TODO add regen logic
} 

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
			func_state_t func_state = get_func_state();
			if (func_state != DRIVING)
			{
				serial_print("Not driving.");
				torque = 0;
				continue;
			}


			drive_state_t drive_state = get_drive_state();

			switch (drive_state)
			{
				case REVERSE:
					linear_accel_to_torque(pedal_data.accelerator_value, &torque);
					break;
				case PIT:
					limit_accel_to_torque(pedal_data.accelerator_value, &torque);
					break;
				case EFFICIENCY:
					paddle_accel_to_torque(pedal_data.accelerator_value, &torque);
					break;
				case PERFORMANCE:
					linear_accel_to_torque(pedal_data.accelerator_value, &torque);
					break;
				default:
					torque = 0;
					break;
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
 

void increase_torque_limit()
{
	if (torque_limit_percentage + 0.1 > 1)
	{
		torque_limit_percentage = 1;
	} else {
		torque_limit_percentage += 0.1;
	}
}

void decrease_torque_limit()
{
	if (torque_limit_percentage - 1 < 0)
	{
		torque_limit_percentage = 0;
	} else {
		torque_limit_percentage -= 0.1;
	}
}