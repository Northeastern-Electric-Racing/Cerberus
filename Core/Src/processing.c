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
#include "nero.h"
#include "can_handler.h"
#include "cerberus_conf.h"
#include "dti.h"
#include "queues.h"
#include "serial_monitor.h"
#include "bms.h"
#include "emrax.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* DO NOT ATTEMPT TO SEND TORQUE COMMANDS LOWER THAN THIS VALUE */
#define MIN_COMMAND_FREQ  60 /* Hz */
#define MAX_COMMAND_DELAY 1000 / MIN_COMMAND_FREQ /* ms */

static float torque_limit_percentage = 1.0;

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
	/* Tell NERO allaboutit */
	osThreadFlagsSet(nero_monitor_handle, NERO_UPDATE_FLAG);
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

static void linear_accel_to_torque(float accel)
{
	/* Sometimes, the pedal travel jumps to 1% even if it is not pressed. */
	if (fabs(accel - 0.01) < 0.001) {
		accel = 0;
	}
	/* Linearly map acceleration to torque */
	int16_t torque = (int16_t)(accel * MAX_TORQUE);
	dti_set_torque(torque);
}

// static void limit_accel_to_torque(float mph, float accel, uint16_t* torque)
// {
// 		static uint16_t torque_accumulator[ACCUMULATOR_SIZE];

// 		uint16_t newVal;
// 		//Results in a value from 0.5 to 0 (at least halving the max torque at all times in pit or reverse)
// 		if (mph > PIT_MAX_SPEED) {
// 			newVal = 0; // If we are going too fast, we don't want to apply any torque to the moving average
// 		}
// 		else {
// 			float torque_derating_factor = fabs(0.5 + ((-0.5/PIT_MAX_SPEED) * mph)); // Linearly derate torque from 0.5 to 0 as speed increases
// 			newVal = accel * torque_derating_factor;
// 		}

// 		// The following code is a moving average filter
// 		uint16_t ave = 0;
// 		uint16_t temp[ACCUMULATOR_SIZE];
// 		ave += newVal; // Add the new value to the sum
// 		ave /= ACCUMULATOR_SIZE; // Divide by the number of values to get the average
// 		temp[0] = newVal; // Add the new value to the array
// 		if(*torque > ave) {
// 			*torque = ave; // If the new value is greater than the average, set the torque to the average
// 		}
// 		memcpy(temp, torque_accumulator, ACCUMULATOR_SIZE); // Copy the new array back to the old array to set the moving average
// }

void increase_torque_limit()
{
	if (torque_limit_percentage + 0.1 > 1) {
		torque_limit_percentage = 1;
	} else {
		torque_limit_percentage += 0.1;
	}
}

void decrease_torque_limit()
{
	if (torque_limit_percentage - 1 < 0) {
		torque_limit_percentage = 0;
	} else {
		torque_limit_percentage -= 0.1;
	}
}

/* Comment out to use single pedal mode */
//#define USE_BRAKE_REGEN 1

/**
 * @brief Torque calculations for efficiency mode. If the driver is braking, do regenerative braking.
 * 
 * @param mc pointer to struct containing dti data
 * @param mph mph of the car
 * @param accel_val adjusted value of the acceleration pedal
 * @param brake_val adjusted value of the brake pedal
 * @param torque pointer to torque value
 */
void handle_endurance(dti_t *mc, float mph, float accel_val, float brake_val)
{
#ifdef USE_BRAKE_REGEN
	// The brake travel ADC value at which we want maximum regen
	static const float travel_scaling_max = 1000;
	if (brake_val > 650 && (mph * 1.609) > 6) {
		// % of max brake pressure * ac current limit
		float brake_current =
			(brake_val / travel_scaling_max) * max_ac_brake;
		if (brake_current > max_ac_brake) {
			// clamp for safety
			brake_current = max_ac_brake;
		}

		// current must be delivered to DTI as a multiple of 10
		dti_set_brake_current((uint16_t)(brake_current * 10));
		*torque = 0;
	} else {
		// accelerating, limit torque
		linear_accel_to_torque(accel_val, torque);
		*torque = (uint16_t)(*torque);
	}
#else
	/* Factor for converting MPH to KMH */
	static const float MPH_TO_KMH = 1.609;

	/* Coefficient to map accel pedal travel % to the max torque */
	static const float coeff = MAX_TORQUE / (1 - ACCELERATION_THRESHOLD);

	/* Pedal is in acceleration range. Set forward torque target. */
	if (accel_val >= ACCELERATION_THRESHOLD) {
		/* Makes acceleration pedal more sensitive since domain is compressed but range is the same */
		uint16_t torque = coeff * accel_val -
				  (accel_val * ACCELERATION_THRESHOLD);

		if (torque > MAX_TORQUE) {
			torque = MAX_TORQUE;
		}

		dti_set_torque(torque);
	} else if (mph * MPH_TO_KMH > 2 && accel_val <= REGEN_THRESHOLD) {
		/* Calculate AC current target for regenerative braking */
		float regen_current = (MAX_REGEN_CURRENT / REGEN_THRESHOLD) *
				      (REGEN_THRESHOLD - accel_val);

		/* Send regen current to motor controller */
		dti_set_regen((uint16_t)(regen_current * 10));
	} else {
		/* Pedal travel is between thresholds, so there should not be acceleration or braking */
		dti_set_torque(0);
	}

#endif
}

osThreadId_t process_pedals_thread;
const osThreadAttr_t torque_calc_attributes = {
	.name = "SendTorque",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime
};

void vProcessPedals(void *pv_params)
{
	const uint16_t delay_time = 5; /* ms */
	/* End application if we try to update motor at freq below this value */
	assert(delay_time < MAX_COMMAND_DELAY);
	pedals_t pedal_data;
	float mph = 0;
	// bool motor_disabled = false;

	dti_t *mc = (dti_t *)pv_params;

	for (;;) {
		osThreadFlagsWait(PEDAL_DATA_FLAG, osFlagsWaitAny,
				  osWaitForever);
		osMessageQueueGet(pedal_data_queue, &pedal_data, 0U,
				  delay_time);

		/* 0.0 - 1.0 */
		float accelerator_value =
			(float)pedal_data.accelerator_value / 100.0;
		float brake_value = (float)pedal_data.brake_value;

		mph = dti_get_mph(mc);

		func_state_t func_state = get_func_state();
		if (func_state != ACTIVE) {
			dti_set_torque(0);
			continue;
		}

		/* EV.4.7: If brakes are engaged and APPS signals more than 25% pedal travel, disable power
			to the motor(s). Re-enable when accelerator has less than 5% pedal travel. */
		//fault_data_t fault_data = { .id = BSPD_PREFAULT, .severity = DEFCON5 };
		/* 600 is an arbitrary threshold to consider the brakes mechanically activated */
		// if (brake_value > 600 && (accelerator_value) > 0.25)
		// {
		// 	printf("\n\n\n\rENTER MOTOR DISABLED\r\n\n\n");
		// 	motor_disabled = true;
		// 	dti_set_torque(0);
		// 	//queue_fault(&fault_data);
		// }

		// if (motor_disabled)
		// {
		// 	printf("\nMotor disabled\n");
		// 	if (accelerator_value < 0.05)
		// 	{
		// 		motor_disabled = false;
		// 		printf("\n\nMotor reenabled\n\n");
		// 	}
		// }

		drive_state_t drive_state = get_drive_state();

		switch (drive_state) {
		// case REVERSE:
		// 	limit_accel_to_torque(mph, accelerator_value, &torque);
		// 	dti_set_torque(torque);
		// 	break;
		// case SPEED_LIMITED:
		// 	limit_accel_to_torque(mph, accelerator_value, &torque);
		// 	break;
		case ENDURANCE:
			handle_endurance(mc, mph, accelerator_value,
					 brake_value);
			break;
		case AUTOCROSS:
			linear_accel_to_torque(accelerator_value);
			break;
		default:
			dti_set_torque(0);
			break;
		}
	}
}
