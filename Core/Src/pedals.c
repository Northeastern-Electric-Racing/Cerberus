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

#include "pedals.h"
#include "state_machine.h"
#include "queues.h"
#include "cerb_utils.h"
#include "nero.h"
#include "can_handler.h"
#include "cerberus_conf.h"
#include "dti.h"
#include "queues.h"
#include "serial_monitor.h"
#include "ams.h"
#include "emrax.h"
#include "monitor.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/* DO NOT ATTEMPT TO SEND TORQUE COMMANDS LOWER THAN THIS VALUE */
#define MIN_COMMAND_FREQ  60 /* Hz */
#define MAX_COMMAND_DELAY 1000 / MIN_COMMAND_FREQ /* ms */

static float torque_limit_percentage = 1.0;

/* Parameters for the pedal monitoring task */
#define MAX_ADC_VAL_12b	  4096
#define PEDAL_DIFF_THRESH 30
#define PEDAL_FAULT_TIME  500 /* ms */

static bool brake_state = false;
osMutexId_t brake_mutex;

enum { ACCELPIN_2, ACCELPIN_1, BRAKEPIN_1, BRAKEPIN_2 };

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

void set_brake_state(bool new_brake_state)
{
	osMutexAcquire(brake_mutex, osWaitForever);
	brake_state = new_brake_state;
	osMutexRelease(brake_mutex);
}

bool get_brake_state()
{
	bool temp;
	osMutexAcquire(brake_mutex, osWaitForever);
	temp = brake_state;
	osMutexRelease(brake_mutex);
	return temp;
}

/**
 * @brief Return the adjusted pedal value based on its offset and maximum value. Clamps negative values to 0.
 * 
 * @param raw the raw pedal value
 * @param offset the offset for the pedal
 * @param max the maximum value of the pedal
 */
uint16_t adjust_pedal_val(uint32_t raw, int32_t offset, int32_t max)
{
	return (int16_t)raw - offset <= 0 ?
		       0 :
		       (uint16_t)(raw - offset) * 100 / (max - offset);
}

/**
 * @brief Callback for pedal fault debouncing.
 * 
 * @param arg The fault message as a char*.
 */
void pedal_fault_cb(void *arg)
{
	fault_data_t fault_data = { .id = ONBOARD_PEDAL_FAULT,
				    .severity = DEFCON1 };
	fault_data.diag = (char *)arg;
	queue_fault(&fault_data);
}

/**
 * @brief Determine if there has been a pedal fault based on pedal sensor data.
 * 
 * @param accel1 Raw accel pedal 1 travel reading
 * @param accel2 Raw accel pedal 2 travel reading
 */
void calc_pedal_faults(uint16_t accel1, uint16_t accel2)
{
	/* oc = Open Circuit */
	static nertimer_t oc_fault_timer;

	/* sc = Short Circuit */
	static nertimer_t sc_fault_timer;

	/* Pedal difference too large fault */
	static nertimer_t diff_fault_timer;

	/* Pedal open circuit fault */
	bool open_circuit = accel1 > (MAX_ADC_VAL_12b - 20) ||
			    accel2 > (MAX_ADC_VAL_12b - 20);
	debounce(open_circuit, &oc_fault_timer, PEDAL_FAULT_TIME,
		 &pedal_fault_cb,
		 "Pedal open circuit fault - max acceleration value");

	/* Pedal short circuit fault */
	bool short_circuit = accel1 < 500 || accel2 < 500;
	debounce(short_circuit, &sc_fault_timer, PEDAL_FAULT_TIME,
		 &pedal_fault_cb,
		 "Pedal short circuit fault - no acceleration value");

	/* Normalize pedal values to be from 0-100 */
	uint16_t accel1_norm =
		adjust_pedal_val(accel1, ACCEL1_OFFSET, ACCEL1_MAX_VAL);
	uint16_t accel2_norm =
		adjust_pedal_val(accel2, ACCEL2_OFFSET, ACCEL2_MAX_VAL);

	/* Pedal difference fault evaluation */
	bool pedals_too_diff = abs(accel1_norm - accel2_norm) >
			       PEDAL_DIFF_THRESH;
	debounce(pedals_too_diff, &diff_fault_timer, PEDAL_FAULT_TIME,
		 &pedal_fault_cb,
		 "Pedal fault - pedal values are too different");
}

/**
 * @brief Function to send raw pedal data over CAN.
 * 
 * @param arg A pointer to an array of 4 unsigned 32 bit integers.
 */
void send_pedal_data(void *arg)
{
	static const uint8_t adc_data_size = 4;
	uint32_t adc_data[adc_data_size];
	/* Copy contents of adc_data to new location in memory because we endian swap them */
	memcpy(adc_data, (uint32_t *)arg, adc_data_size * sizeof(adc_data[0]));

	can_msg_t accel_pedals_msg = { .id = CANID_PEDALS_ACCEL_MSG,
				       .len = 8,
				       .data = { 0 } };
	can_msg_t brake_pedals_msg = { .id = CANID_PEDALS_BRAKE_MSG,
				       .len = 8,
				       .data = { 0 } };

	endian_swap(&adc_data[ACCELPIN_1], sizeof(adc_data[ACCELPIN_1]));
	endian_swap(&adc_data[ACCELPIN_2], sizeof(adc_data[ACCELPIN_2]));
	memcpy(accel_pedals_msg.data, &adc_data, accel_pedals_msg.len);
	queue_can_msg(accel_pedals_msg);

	endian_swap(&adc_data[BRAKEPIN_1], sizeof(adc_data[BRAKEPIN_1]));
	endian_swap(&adc_data[BRAKEPIN_2], sizeof(adc_data[BRAKEPIN_2]));
	memcpy(brake_pedals_msg.data, adc_data + 2, brake_pedals_msg.len);
	queue_can_msg(brake_pedals_msg);
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
	static fault_data_t fault_data = { .id = BSPD_PREFAULT,
					   .severity = DEFCON5,
					   .diag = "BSPD prefault triggered" };
	static bool motor_disabled = false;

	/* EV.4.7: If brakes are engaged and APPS signals more than 25% pedal travel, disable power
	to the motor(s). Re-enable when accelerator has less than 5% pedal travel. */

	/* BSPD braking theshold is arbitrary */
	if (brake_val > 700 && accel_val > 0.25) {
		motor_disabled = true;
		queue_fault(&fault_data);
	}

	if (motor_disabled) {
		if (accel_val < 0.05) {
			motor_disabled = false;
		} else {
			dti_set_torque(0);
		}
	}

	return motor_disabled;
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
	float brake_current =
		(brake_val / travel_scaling_max) * MAX_REGEN_CURRENT;
	if (brake_current > MAX_REGEN_CURRENT) {
		// clamp for safety
		brake_current = MAX_REGEN_CURRENT;
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
	static const float coeff = MAX_TORQUE / (1 - ACCELERATION_THRESHOLD);

	/* Makes acceleration pedal more sensitive since domain is compressed but range is the same */
	uint16_t torque =
		coeff * accel_val - (accel_val * ACCELERATION_THRESHOLD);

	if (torque > MAX_TORQUE) {
		torque = MAX_TORQUE;
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
	float regen_current = (MAX_REGEN_CURRENT / REGEN_THRESHOLD) *
			      (REGEN_THRESHOLD - accel_val);

	if (regen_current > MAX_REGEN_CURRENT) {
		regen_current = MAX_REGEN_CURRENT;
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
void handle_endurance(dti_t *mc, float mph, float accel_val, float brake_val)
{
#ifdef USE_BRAKE_REGEN
	if (brake_val > 650 && (mph * 1.609) > 5) {
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
	dti_t *mc = args->mc;
	pdu_t *pdu = args->pdu;
	free(args);

	uint32_t adc_data[4];
	osTimerId_t send_pedal_data_timer =
		osTimerNew(&send_pedal_data, osTimerPeriodic, adc_data, NULL);

	/* Send CAN messages with raw pedal readings, we do not care if it fails*/
	osTimerStart(send_pedal_data_timer, 100);

	/* Mutexes for setting and getting pedal values and brake state */
	brake_mutex = osMutexNew(NULL);

	const uint16_t delay_time = 10; /* ms */
	/* End application if we try to update motor at freq below this value */
	assert(delay_time < MAX_COMMAND_DELAY);

	for (;;) {
		read_pedals(mpu, adc_data);

		uint32_t accel1_raw = adc_data[ACCELPIN_1];
		uint32_t accel2_raw = adc_data[ACCELPIN_2];

		calc_pedal_faults(accel1_raw, accel2_raw);

		/* Normalize pedal values to be from 0-100 */
		uint16_t accel1_norm = adjust_pedal_val(
			accel1_raw, ACCEL1_OFFSET, ACCEL1_MAX_VAL);
		uint16_t accel2_norm = adjust_pedal_val(
			accel2_raw, ACCEL2_OFFSET, ACCEL2_MAX_VAL);

		/* Combine normalized values from both accel pedal sensors */
		uint16_t accel_val = (uint16_t)(accel1_norm + accel2_norm) / 2;
		uint16_t brake_val =
			(adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2;

		/* Turn brakelight on or off */
		write_brakelight(pdu, brake_val > PEDAL_BRAKE_THRESH);
		set_brake_state(brake_val > PEDAL_BRAKE_THRESH);

		/* 0.0 - 1.0 */
		float accelerator_value = (float)accel_val / 100.0;

		if (calc_bspd_prefault(accelerator_value, brake_val)) {
			/* Prefault triggered */
			osDelay(delay_time);
			continue;
		}

		float mph = dti_get_mph(mc);
		func_state_t func_state = get_func_state();

		switch (func_state) {
		case F_EFFICIENCY:
			handle_endurance(mc, mph, accelerator_value, brake_val);
			break;
		case F_PERFORMANCE:
			linear_accel_to_torque(accelerator_value);
			break;
		case F_PIT:
			handle_pit(mph, accelerator_value);
			break;
		case REVERSE:
			handle_reverse(mph, accelerator_value);
			break;
		default:
			dti_set_torque(0);
			break;
		}

		osDelay(delay_time);
	}
}