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

#include "cmsis_os.h"
#include "stdbool.h"
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
 * @brief Get state of brakes
 * 
 * @return true Brakes engaged
 * @return false Brakes not engaged
 */
bool get_brake_state();

/**
 * @brief Task for reading pedal data, calculating pedal faults, and sending drive commands to the DTI.
 * 
 * @param pv_params Pointer to pedals_args_t
 */
void vProcessPedals(void *pv_params);
extern osThreadId_t process_pedals_thread;
extern const osThreadAttr_t process_pedals_attributes;

#endif
