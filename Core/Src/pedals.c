/**
 * @file processing.c
 * @brief Definitions for tasks for processing data.
 * @version 0.1
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "pedals.h"
#include "state_machine.h"
#include "debounce.h"
#include "can_handler.h"
#include "cerberus_conf.h"
#include "dti.h"
#include "emrax.h"
#include "monitor.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fault.h"
#include "state_machine.h"

/* DO NOT ATTEMPT TO SEND TORQUE COMMANDS LOWER THAN THIS VALUE */
#define MIN_COMMAND_FREQ  60 /* Hz */
#define MAX_COMMAND_DELAY 1000 / MIN_COMMAND_FREQ /* ms */

#define REGEN_INCREMENT_STEP 5 /* AC Amps */

float torque_limit_percentage = 1.0;
uint8_t regen_limit = 20;
static bool launch_control_enabled = false;

/* Parameters for the pedal monitoring task */
#define MAX_ADC_VAL_12b	   4096
#define MAX_VOLTS	   3.3 /* volts */
#define MAX_VOLTS_UNSCALED 5.0

#define PEDAL_DIFF_THRESH 0.20 /* percentage */
#define PEDAL_FAULT_TIME  90 /* ms */

#define APPS_THRESHOLD_BUF 0.45
enum { ACCELPIN_1, ACCELPIN_2, BRAKEPIN_1, BRAKEPIN_2 };

static bool brake_pressed = false;
static osMutexId_t brake_state_mut;
static osMutexAttr_t brake_mutex_attributes;

bool get_brake_state()
{
	bool temp;
	osMutexAcquire(brake_state_mut, osWaitForever);
	temp = brake_pressed;
	osMutexRelease(brake_state_mut);
	return temp;
}

/**
 * @brief Converts the adc to the voltage out of 5V (for rules)
 * 
 * @param raw_adc 
 * @return float 
 */
static float adc_to_volts(uint32_t raw_adc)
{
	float v3_volts = raw_adc * MAX_VOLTS / MAX_ADC_VAL_12b;
	// undo 2k + 3k voltage divider on APPS lines
	return ((2000.0 + 3000) / 3000) * v3_volts;
}

/**
 * @brief Return the adjusted pedal value based on its offset. Clamps negative values to 0.
 * 
 * @param voltage the raw pedal value
 * @param offset the offset for the pedal
 */
static float pedal_percent_pressed(float voltage, float offset, float max)
{
	return voltage - offset < 0 ? 0 : (voltage - offset) / (max - offset);
}

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
	if (torque_limit_percentage - 0.1 < 0) {
		torque_limit_percentage = 0;
	} else {
		torque_limit_percentage -= 0.1;
	}
}

void increase_regen_limit()
{
	if (regen_limit + REGEN_INCREMENT_STEP > MAX_REGEN_CURRENT) {
		regen_limit = MAX_REGEN_CURRENT;
	} else {
		regen_limit += REGEN_INCREMENT_STEP;
	}
}

void decrease_regen_limit()
{
	if (regen_limit - 0.1 < 0) {
		regen_limit = 0;
	} else {
		regen_limit -= REGEN_INCREMENT_STEP;
	}
}

void set_regen_limit(uint8_t limit)
{
	regen_limit = limit;

	if (regen_limit > MAX_REGEN_CURRENT) {
		regen_limit = MAX_REGEN_CURRENT;
	} else if (regen_limit < 0.0) {
		regen_limit = 0.0;
	}
}

void set_torque_limit(float percentage)
{
	torque_limit_percentage = percentage;

	// Make sure the percentage is within the valid range
	if (torque_limit_percentage > 1.0) {
		torque_limit_percentage = 1.0;
	} else if (torque_limit_percentage < 0.0) {
		torque_limit_percentage = 0.0;
	}
}

float get_torque_limit_percentage()
{
	return torque_limit_percentage;
}

uint8_t get_regen_limit()
{
	return regen_limit;
}

void toggle_launch_control()
{
	launch_control_enabled = !launch_control_enabled;
}

bool get_launch_control_enabled()
{
	return launch_control_enabled;
}

/**
 * @brief Callback for pedal fault debouncing.
 * 
 * @param arg The fault message as a char*.
 */
void pedal_fault_cb(void *arg)
{
	fault_data_t fault_data = {
		.fault_id = ONBOARD_PEDAL_FAULT,
	};
	fault_data.diag = (char *)arg;
	queue_fault(&fault_data);
}

/**
 * @brief Determine if there has been a pedal fault based on pedal sensor data.
 * 
 * @param accel1 pedal 1 travel voltage reading 
 * @param accel2 pedal 2 travel voltage reading 
 */
bool calc_pedal_faults(float accel1, float accel2, float accel1_norm,
		       float accel2_norm)
{
	/* oc = Open Circuit */
	static nertimer_t oc_fault_timer;

	/* sc = Short Circuit */
	static nertimer_t sc_fault_timer;

	/* Pedal difference too large fault */
	static nertimer_t diff_fault_timer;

	/* EV3.5.4: For analog acceleration control signals, this error checking must detect open circuit, short to 
	ground and short to sensor power. */

	/* Pedal open circuit fault */
	bool open_circuit = accel1 > MAX_VOLTS_UNSCALED - APPS_THRESHOLD_BUF ||
			    accel2 > MAX_VOLTS_UNSCALED - APPS_THRESHOLD_BUF;
	debounce(open_circuit, &oc_fault_timer, PEDAL_FAULT_TIME,
		 &pedal_fault_cb,
		 "Pedal open circuit fault - max acceleration value");

	/* Pedal short circuit to gnd */
	bool short_circuit = accel1 < MIN_APPS1_VOLTS - APPS_THRESHOLD_BUF ||
			     accel2 < MIN_APPS2_VOLTS - APPS_THRESHOLD_BUF;
	debounce(short_circuit, &sc_fault_timer, PEDAL_FAULT_TIME,
		 &pedal_fault_cb,
		 "Pedal grounded circuit fault - no acceleration value");

	/* Pedal difference fault evaluation */
	// Fault registered when greater than 10% is detected between the sensor readings
	// to detect a short between the two sensors (outlined in 2025 ESF)
	// printf("ACCEL1 NORM %f", accel1_norm);
	// printf("ACCEL2 NORM %f", accel2_norm);
	bool pedals_too_diff = fabs(accel1_norm - accel2_norm) >
			       PEDAL_DIFF_THRESH;

	debounce(pedals_too_diff, &diff_fault_timer, PEDAL_FAULT_TIME,
		 &pedal_fault_cb,
		 "Pedal short fault - pedal values are too different");

	if (open_circuit || short_circuit || pedals_too_diff) {
		return true;
	}
	return false;
}

/**
 * @brief Function to send raw pedal data over CAN.
 * 
 * @param arg A pointer to an array of 4 unsigned 32 bit integers.
 */
void send_pedal_data(void *arg)
{
	uint32_t *adc_data = (uint32_t *)arg;

	can_msg_t pedals_msg = { .id = CANID_PEDALS_MSG,
				 .len = 8,
				 .data = { 0 } };

	struct __attribute__((__packed__)) {
		uint16_t accel_1;
		uint16_t accel_2;
		uint16_t brake_1;
		uint16_t brake_2;
	} voltage_data;

	voltage_data.accel_1 =
		(uint16_t)(adc_to_volts(adc_data[ACCELPIN_1]) * 100);
	voltage_data.accel_2 =
		(uint16_t)(adc_to_volts(adc_data[ACCELPIN_2]) * 100);
	voltage_data.brake_1 =
		(uint16_t)(adc_to_volts(adc_data[BRAKEPIN_1]) * 100);
	voltage_data.brake_2 =
		(uint16_t)(adc_to_volts(adc_data[BRAKEPIN_2]) * 100);

	endian_swap(&voltage_data.accel_1, sizeof(voltage_data.accel_1));
	endian_swap(&voltage_data.accel_2, sizeof(voltage_data.accel_2));
	endian_swap(&voltage_data.brake_1, sizeof(voltage_data.brake_1));
	endian_swap(&voltage_data.brake_2, sizeof(voltage_data.brake_2));

	memcpy(pedals_msg.data, &voltage_data, pedals_msg.len);
	queue_can_msg(pedals_msg);
}

/**
 * @brief Determine if power to the motor controller should be disabled based on brake and accelerator pedal travel.
 * 
 * @param accel_val Percent travel of the accelerator pedal from 0-1
 * @param brake_val Brake pressure sensor reading
 * @return bool True for prefault conditions met, false for no prefault
 */
bool calc_bspd_prefault(float accel_val, float brake_val)
{
	static fault_data_t fault_data = { .fault_id = BSPD_PREFAULT,
					   .diag = "BSPD prefault triggered" };
	static bool motor_disabled = false;

	/* EV.4.7: If brakes are engaged and APPS signals more than 25% pedal travel, disable power
	to the motor(s). Re-enable when accelerator has less than 5% pedal travel. */

	if (brake_val > PEDAL_BRAKE_THRESH && accel_val > 0.25) {
		motor_disabled = true;
		queue_fault(&fault_data);
	}

	if (motor_disabled) {
		if (accel_val < 0.05) {
			motor_disabled = false;
		}
	}

	return motor_disabled;
}

#ifndef POWER_REGRESSION_PEDAL_TORQUE_TRANSFER
static void linear_accel_to_torque(float accel)
{
	/* Sometimes, the pedal travel jumps to 3% even if it is not pressed. */
	if (accel < 0.03) {
		accel = 0.0;
	}
	if (accel > 1) {
		accel = 1.0;
	}

	/* Linearly map acceleration to torque */
	int16_t torque = (int16_t)(accel * MAX_TORQUE);

	dti_set_torque(torque);
}

#else
static void power_regression_accel_to_torque(float accel)
{
	/* Sometimes, the pedal travel jumps to 1% even if it is not pressed. */
	if (fabs(accel - 0.01) < 0.001) {
		accel = 0;
	}
	/*  map acceleration to torque */
	int16_t torque =
		(int16_t)(0.137609 * powf(accel, 1.43068) * MAX_TORQUE);
	/* These values came from creating a power regression function intersecting three points: (0,0) (20,10) & (100,100)*/

	dti_set_torque(torque);
}
#endif

/**
 * @brief Derate torque target to keep car below the maximum pit/reverse mode speed.
 * 
 * @param mph Speed of the car
 * @param accel Percent travel of the acceleration pedal from 0-1
 * @return int16_t Derated torque
 */
static int16_t derate_torque(float mph, float accel)
{
	static int16_t torque_accumulator[ACCUMULATOR_SIZE];
	/* index in moving average */
	static uint8_t index = 0;

	int16_t torque;

	/* If we are going too fast, we don't want to apply any torque to the moving average */
	if (mph > PIT_MAX_SPEED) {
		torque = 0;
	} else {
		/* Highest torque % in pit mode */
		static const float max_torque_percent = 0.3;
		/* Linearly derate torque from 30% to 0% as speed increases */
		float torque_derating_factor =
			max_torque_percent -
			(max_torque_percent / PIT_MAX_SPEED);
		accel *= torque_derating_factor;
		torque = MAX_TORQUE * accel;
	}

	/* Add value to moving average */
	torque_accumulator[index] = torque;
	index = (index + 1) % ACCUMULATOR_SIZE;

	/* Get moving average then send torque command to dti motor controller */
	int16_t sum = 0;
	for (uint8_t i = 0; i < ACCUMULATOR_SIZE; i++) {
		sum += torque_accumulator[i];
	}
	return sum / ACCUMULATOR_SIZE;
}

/**
 * @brief Drive forward with a speed limit of 5 mph.
 * 
 * @param mph Current speed of the car.
 * @param accel % pedal travel of the accelerator pedal.
 */
static void handle_pit(float mph, float accel)
{
	dti_set_torque(derate_torque(mph, accel));
}

/**
 * @brief Drive in speed limited reverse mode.
 * 
 * @param mph Current speed of the car.
 * @param accel % pedal travel of the accelerator pedal.
 */
static void handle_reverse(float mph, float accel)
{
	dti_set_torque(-1 * derate_torque(mph, accel));
}

/* Comment out to use single pedal mode */
//#define USE_BRAKE_REGEN 1

/**
 * @brief Calculate and send regen braking AC current target based on brake pedal travel.
 * 
 * @param brake_val The reading of the brake pressure sensors.
 */
void brake_pedal_regen(float brake_val)
{
	// The brake travel ADC value at which we want maximum regen
	static const float travel_scaling_max = 1000;
	// % of max brake pressure * ac current limit
	float brake_current = (brake_val / travel_scaling_max) * regen_limit;
	if (brake_current > regen_limit) {
		// clamp for safety
		brake_current = regen_limit;
	}

	// current must be delivered to DTI as a multiple of 10
	dti_send_brake_current((uint16_t)(brake_current * 10));
}

/**
 * @brief Calculate and send torque command to motor controller.
 * 
 * @param accel_val Accelerator pedal percent travel from 0-1
 */
void accel_pedal_regen_torque(float accel_val)
{
	/* Coefficient to map accel pedal travel % to the max torque */
	float coeff = (MAX_TORQUE * torque_limit_percentage) /
		      (1 - ACCELERATION_THRESHOLD);

	/* Makes acceleration pedal more sensitive since domain is compressed but range is the same */
	uint16_t torque =
		coeff * accel_val - (accel_val * ACCELERATION_THRESHOLD);

	/* Limit torque percentage wise in endurance mode */
	if (torque > MAX_TORQUE * torque_limit_percentage) {
		torque = MAX_TORQUE * torque_limit_percentage;
	}

	dti_set_torque(torque);
}

/**
 * @brief Calculate regen braking AC current target based on accelerator pedal percent travel.
 * 
 * @param accel_val Accelerator pedal percent travel from 0-1
 */
void accel_pedal_regen_braking(float accel_val)
{
	/* Calculate AC current target for regenerative braking */
	float regen_current =
		(regen_limit / REGEN_THRESHOLD) * (REGEN_THRESHOLD - accel_val);

	if (regen_current > regen_limit) {
		regen_current = regen_limit;
	}

	/* Send regen current to motor controller */
	dti_set_regen((uint16_t)(regen_current * 10));
}

/**
 * @brief Torque calculations for efficiency mode. If the driver is braking, do regenerative braking.
 * 
 * @param mc pointer to struct containing dti data
 * @param mph mph of the car
 * @param accel_val adjusted value of the acceleration pedal
 * @param brake_val adjusted value of the brake pedal
 * @param torque pointer to torque value
 */
void handle_endurance(float mph, float accel_val, float brake_val)
{
#ifdef USE_BRAKE_REGEN
	if (brake_val > PEDAL_BRAKE_THRESH && (mph * 1.609) > 5) {
		brake_pedal_regen(brake_val);
	} else {
		// accelerating, limit torque
		linear_accel_to_torque(accel_val, torque);
	}
#else
	/* Factor for converting MPH to KMH */
	static const float MPH_TO_KMH = 1.609;

	/* Pedal is in acceleration range. Set forward torque target. */
	if (accel_val >= ACCELERATION_THRESHOLD) {
		accel_pedal_regen_torque(accel_val);
	} else if (mph * MPH_TO_KMH > 2 && accel_val <= REGEN_THRESHOLD) {
		accel_pedal_regen_braking(accel_val);
	} else {
		/* Pedal travel is between thresholds, so there should not be acceleration or braking */
		dti_set_torque(0);
	}

#endif
}

const float deltaMPHPS_max =
	22.0f; // Miles per hour per second, based on matlab accel numbers

void handle_launch_control(float mph, float accel_val)
{
	static float last_mph = 0.0f;
	static uint32_t prevTime = 0;

	if (prevTime == 0) { // Initialize time
		prevTime = HAL_GetTick();
		return;
	}

	uint32_t now = HAL_GetTick();
	uint32_t delta_ms = now - prevTime;

	float delta_mph = mph - last_mph;
	float max_delta_adjusted = deltaMPHPS_max * (delta_ms / 1000.0f);

	if (delta_mph > max_delta_adjusted) {
		dti_set_torque(0);
	} else {
		linear_accel_to_torque(accel_val);
	}

	// Update for next cycle
	prevTime = now;
	last_mph = mph;
}

osThreadId_t process_pedals_thread;
const osThreadAttr_t process_pedals_attributes = {
	.name = "PedalMonitor",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime,
};

void vProcessPedals(void *pv_params)
{
	pedals_args_t *args = (pedals_args_t *)pv_params;
	mpu_t *mpu = args->mpu;
	assert(mpu);
	dti_t *mc = args->mc;
	assert(mc);
	pdu_t *pdu = args->pdu;
	assert(pdu);

	free(args);

	uint32_t adc_data[4];
	osTimerId_t send_pedal_data_timer =
		osTimerNew(&send_pedal_data, osTimerPeriodic, adc_data, NULL);

	/* Send CAN messages with raw pedal readings, we do not care if it fails*/
	osTimerStart(send_pedal_data_timer, 100);

	const uint16_t delay_time = 10; /* ms */
	/* End application if we try to update motor at freq below this value */
	//assert(delay_time < MAX_COMMAND_DELAY);

	brake_state_mut = osMutexNew(&brake_mutex_attributes);

	for (;;) {
		read_pedals(mpu, adc_data);

		float accel1_volts = adc_to_volts(adc_data[ACCELPIN_1]);
		float accel2_volts = adc_to_volts(adc_data[ACCELPIN_2]);

		// printf("accel1 volts %f\n", accel1_volts);
		// printf("accel2 volts %f\n", accel2_volts);

		/* Normalize pedal values to be from 0-1 */
		float accel1_norm = pedal_percent_pressed(
			accel1_volts, MIN_APPS1_VOLTS, MAX_APPS1_VOLTS);
		float accel2_norm = pedal_percent_pressed(
			accel2_volts, MIN_APPS2_VOLTS, MAX_APPS2_VOLTS);

		bool possible_faults = calc_pedal_faults(
			accel1_volts, accel2_volts, accel1_norm, accel2_norm);

		/* same for brake values */
		float brake_avg =
			(adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2;

		// printf("brake1 volts %f\n", adc_to_volts(adc_data[BRAKEPIN_1]));
		// printf("brake2 volts %f\n", adc_to_volts(adc_data[BRAKEPIN_2]));
		/* calc percent brake is pressed */
		float brake_value = pedal_percent_pressed(
			adc_to_volts(brake_avg), 0, MAX_VOLTS_UNSCALED);
		float accel_value = (accel1_norm + accel2_norm) / 2;

		/* Turn brakelight on or off */
		osMutexAcquire(brake_state_mut, osWaitForever);
		if (brake_value > PEDAL_BRAKE_THRESH) {
			brake_pressed = true;
		} else {
			brake_pressed = false;
		}
		osMutexRelease(brake_state_mut);
		write_brakelight(pdu, brake_pressed);

		if (calc_bspd_prefault(accel_value, brake_value)) {
			/* Prefault triggered */
			// osDelay(delay_time);
			// dti_set_torque(0);
			// continue;
		}

		float mph = dti_get_mph(mc);
		func_state_t func_state = get_func_state();

		switch (func_state) {
		case F_EFFICIENCY:
			handle_endurance(mph, accel_value, brake_value);
			break;
		case F_PERFORMANCE:
			if (launch_control_enabled) {
				handle_launch_control(mph, accel_value);
			} else {
#ifndef POWER_REGRESSION_PEDAL_TORQUE_TRANSFER
				linear_accel_to_torque(accel_value);
#else
				power_regression_accel_to_torque(accel_value);
#endif
			}
			break;
		case F_PIT:
			handle_pit(mph, accel_value);
			break;
		case REVERSE:
			handle_reverse(mph, accel_value);
			break;
		default:
			dti_set_torque(0);
			break;
		}
		osDelay(delay_time);
	}
}
