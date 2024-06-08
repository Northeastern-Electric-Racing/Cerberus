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
#include "torque.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "bms.h"


/* DO NOT ATTEMPT TO SEND TORQUE COMMANDS LOWER THAN THIS VALUE */
#define MIN_COMMAND_FREQ  60					  /* Hz */
#define MAX_COMMAND_DELAY 1000 / MIN_COMMAND_FREQ /* ms */

static float torque_limit_percentage = 1.0;


osThreadId_t torque_calc_handle;
const osThreadAttr_t torque_calc_attributes = { .name		= "SendTorque",
												.stack_size = 128 * 8,
												.priority = (osPriority_t)osPriorityRealtime2 };

static void linear_accel_to_torque(float accel, int16_t* torque)
{
	/* Linearly map acceleration to torque */
	*torque = (int16_t)(accel * MAX_TORQUE);
}

static float rpm_to_mph(uint32_t rpm)
{
	/* Convert RPM to MPH */
	// rpm * gear ratio = wheel rpm
	// tire diamter (in) to miles --> tire diamter miles
	// wheel rpm * 60 --> wheel rph
	// tire diamter miles * pi --> tire circumference
	// rph * wheel circumference miles --> mph
	return (rpm / (GEAR_RATIO))*60 * (TIRE_DIAMETER / 63360.0)*M_PI;
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


// comment out to use single pedal mode
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
void handle_endurance(dti_t* mc, float mph, float accel_val, float brake_val, float accel_relative_val, int16_t* torque) {
	// max ac current to brake with
	static const float max_ac_brake = 25; // Amps AC
	#ifdef USE_BRAKE_REGEN
		// The brake travel ADC value at which we want maximum regen
		static const float travel_scaling_max = 1000;
		if (brake_val > 650 && (mph*1.609) > 6 ) {
			// % of max brake pressure * ac current limit
			float brake_current = (brake_val / travel_scaling_max) * max_ac_brake;
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
			*torque = (uint16_t) (*torque);
		}
	#else
		// percent of accel pedal to be used for regen
		static const float start_regen = 0.2;
		/**
		 * regen_factor: coeffecient in linear fit of % pedal travel to torque. torque = regen_factor * accel_val
		 */
		static const float regen_factor = max_ac_brake / (start_regen * 100); // like max torque but instead dictates max regen

		float moved_accel = (accel_val - (start_regen * 100 ));
		// if the rescaled accel is positive then convert it to torque, and full send
		if (moved_accel >= 0) {
			/* Pedal more sensitive but in return max torque doesn't change */
			moved_accel *= (MAX_TORQUE * (1.0 / (1.0 - start_regen)));
			linear_accel_to_torque(moved_accel, torque);
		}
		else { 	// if the rescaled accel is negative, then use a different conversion factor
			// use scale to get the 
			float regen_torque = (moved_accel*-1.0*regen_factor) * ((1.0 / start_regen) + (1.0 / (1.0 - start_regen)));

			// clamp to max
			if ((regen_torque/0.61)/10 > max_ac_brake) {
				regen_torque = max_ac_brake * 0.61 * 10.0;	
			}

			// pass in torque to the dti code
			dti_set_regen(regen_torque / 10.0);
			// do not move forward
			*torque = 0;
		}
		
	#endif
}

void vCalcTorque(void* pv_params)
{
	const uint16_t delay_time = 5; /* ms */
	/* End application if we try to update motor at freq below this value */
	assert(delay_time < MAX_COMMAND_DELAY);
	pedals_t pedal_data;
	int16_t torque = 0;
	float mph = 0;
	osStatus_t stat;
	bool motor_disabled = false;

	dti_t *mc = (dti_t *)pv_params;

	for (;;) {
		stat = osMessageQueueGet(pedal_data_queue, &pedal_data, 0U, delay_time);

		float accelerator_value = (float) pedal_data.accelerator_value;
		float brake_value = (float) pedal_data.brake_value;

		/* If we receive a new message within the time frame, calc new torque */
		if (stat == osOK)
		{	
			uint32_t rpm = dti_get_rpm(mc);
			mph = rpm_to_mph(rpm);
			set_mph(mph);

			func_state_t func_state = get_func_state();
			if (func_state != ACTIVE)
			{
				torque = 0;
				continue;
			}

			/* EV.4.7: If brakes are engaged and APPS signals more than 25% pedal travel, disable power
			to the motor(s). Re-enable when accelerator has less than 5% pedal travel. */

			fault_data_t fault_data = { .id = BSPD_PREFAULT, .severity = DEFCON5 };
			float rel_accel_pedal_travel = accelerator_value / (((ACCEL1_MAX_VAL - ACCEL1_OFFSET) + (ACCEL2_MAX_VAL - ACCEL2_OFFSET)) / 2.0);
			uint16_t brakes_engaged_threshold = 650;
			if (brake_value > brakes_engaged_threshold && rel_accel_pedal_travel > 0.25) 
			{
				motor_disabled = true;
				queue_fault(&fault_data);
			}

			if (motor_disabled) 
			{
				if (rel_accel_pedal_travel < 0.05) 
				{
					motor_disabled = false;
					continue;
				} else {
					torque = 0;
					dti_set_torque(torque);
					continue;
				}
			}

			drive_state_t drive_state = get_drive_state();

			switch (drive_state)
			{
				// case REVERSE:
				// 	limit_accel_to_torque(mph, accelerator_value, &torque);
				// 	dti_set_torque(torque);
				// 	break;
				// case SPEED_LIMITED:
				// 	limit_accel_to_torque(mph, accelerator_value, &torque);
				// 	break;
				case ENDURANCE:
					handle_endurance(mc, mph, accelerator_value, brake_value, rel_accel_pedal_travel, &torque);
					break;
				case AUTOCROSS:
					linear_accel_to_torque(accelerator_value, &torque);
					break;
				default:
					torque = 0;
					break;
			}

			// serial_print("accel val: %d\r\n", pedal_data.accelerator_value);

			serial_print("torque: %d\r\n", torque);
			/* Send whatever torque command we have on record */
			dti_set_torque(torque);
		}
	}
}
