/**
 * @file pedals.h
 * @brief Functions and tasks for processing data.
 * @version 0.1
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef PROCESSING_H
#define PROCESSING_H

#include "dti.h"
#include "pdu.h"
#include "mpu.h"

#define PEDAL_DATA_FLAG 1U

#define PIT_MAX_SPEED	 5.0 /* mph */
#define ACCUMULATOR_SIZE 10 /* size of the accumulator for averaging */

typedef struct {
	uint16_t brake_value;
	uint16_t accelerator_value; /* 0-100 */
} pedals_t;

typedef struct {
	mpu_t *mpu;
	dti_t *mc;
	pdu_t *pdu;
} pedals_args_t;

/*
 * Increases the torque limit by 10%
*/
void increase_torque_limit();

/*
 * Decreases the torque limit by 10%
*/
void decrease_torque_limit();

/**
 * @brief Sets the torque limit to a specific percentage
 * 
 * @param percentage Percentage of the torque limit. Acceptable values are between 0.0 and 1.0.
 * 
 */
void set_torque_limit(float percentage);

/**
 * @brief Get the current torque limit percentage
 * 
 * @return torque limit percentage
 */
float get_torque_limit_percentage();

/*
 * Increases the regen limit by the REGEN_INCREMENT_STEP
*/
void increase_regen_limit();

/*
 * Decreases the regen limit by the REGEN_INCREMENT_STEP
*/
void decrease_regen_limit();

/**
 * @brief Sets the regen limit to a specific value
 * 
 * @param limit The new regen limit. Acceptable values are between 0 and MAX_REGEN_CURRENT
 * 
 */
void set_regen_limit(uint8_t limit);

/**
 * @brief Get the current regen limit
 * 
 * @return regen limit
 */
uint8_t get_regen_limit();

/**
 * @brief Task for reading pedal data, calculating pedal faults, and sending drive commands to the DTI.
 * 
 * @param pv_params Pointer to pedals_args_t
 */
void vProcessPedals(void *pv_params);
extern osThreadId_t process_pedals_thread;
extern const osThreadAttr_t process_pedals_attributes;

/**
 * @brief reads whether the brake is currently engaged
 * 
 * @return true if brake engaged, false otherwise
 */
bool get_brake_state();

/**
 * @brief Toggles launch control for performance mode
 */
void toggle_launch_control();

/**
 * @brief Returns the current state of launch control
 */
bool get_launch_control();

#endif
