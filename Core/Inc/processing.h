/**
 * @file processing.h
 * @author Scott Abramson
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

#define TSMS_UPDATE_FLAG 1U
#define PEDAL_DATA_FLAG	 1U

#define PIT_MAX_SPEED	 5.0 /* mph */
#define ACCUMULATOR_SIZE 10 /* size of the accumulator for averaging */

/*
 * Increases the torque limit by 10%
*/
void increase_torque_limit();

/*
 * Decreases the torque limit by 10%
*/
void decrease_torque_limit();

/**
 * @brief Get the debounced TSMS reading.
 * 
 * @return State of TSMS.
 */
bool get_tsms();

/**
 * @brief Task for processing data from the TSMS.
 */
void vProcessTSMS(void *pv_params);
extern osThreadId_t process_tsms_thread_id;
extern const osThreadAttr_t process_tsms_attributes;

typedef struct {
	dti_t *mc;
	pdu_t *pdu;
} process_pedals_args_t;

/**
 * @brief Task for processing pedal data.
 */
void vProcessPedals(void *pv_params);
extern osThreadId_t process_pedals_thread;
extern const osThreadAttr_t torque_calc_attributes;

#endif
