/**
 * @file control.h
 * @author Daniel Nakhooda
 * @brief Handles the states for similar peripheral devices (fan battery box , pumps, radiator fan)
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2025
 */

#ifndef CONTROL_H
#define CONTROL_H

#include "can.h"
#include "debounce.h"
#include "dti.h"
#include "pdu.h"

#define CONTROL_CANID_FANBATTBOX 0x4A1
#define CONTROL_CANID_PUMP	 0x4A0
#define CONTROL_CANID_RADFAN	 0x499

extern osThreadId_t control_handle;
extern const osThreadAttr_t control_attributes;

typedef int8_t (*control_func_t)(pdu_t *pdu, bool state);

typedef enum {
	DEVICE_PUMP0,
	DEVICE_PUMP1,
	DEVICE_RADFAN0,
	DEVICE_RADFAN1,
	DEVICE_FANBATTBOX,
	NUM_DEVICES,
} device_type_t;

// sub-struct holding upper and lower temperature bounds.
typedef struct {
	uint8_t upper_motor_temp_bound;
	uint8_t lower_motor_temp_bound;
} device_temp_bounds_t;

// struct holding the temperature bounds for a single device for each possible state.
typedef struct {
	device_temp_bounds_t ready;
	device_temp_bounds_t f_reverse;
	device_temp_bounds_t f_pit;
	device_temp_bounds_t f_performance;
	device_temp_bounds_t f_efficiency;
	device_temp_bounds_t faulted;
	device_temp_bounds_t standard;
} device_config_t;

/* Holds all the information needed to determine and set the state of a device */
typedef struct {
	pdu_t *pdu;
	control_func_t control_func; /* function to set device state */
	device_type_t device_type; /* Device Type (Pump or Radfan) */
	nertimer_t timer; /* Debounce Timer */
	device_config_t temp_bounds; /* temperature bounds */
} device_control_t;

/* Holds arguments for control thread */
typedef struct {
	pdu_t *pdu;
	dti_t *mc;
} control_args_t;

/**
 * @brief Main control loop
 *
 * @param params Pointer to control_args_t struct
 */
void vControl(void *params);

/**
 * @brief Records the fan battbox state sent through CAN
 *
 * @param calypso_states Pointer calypso states struct
 * @param msg CAN message sent
 */
void control_fanbattbox_record(can_msg_t msg);

/**
 * @brief Records pump0 and pump1 states sent through CAN
 *
 * @param calypso_states Pointer calypso states struct
 * @param msg CAN message sent
 */
void control_pump_record(can_msg_t msg);

/**
 * @brief Records radfan1 and radfan2 states sent through CAN
 *
 * @param calypso_states Pointer calypso states struct
 * @param msg CAN message sent
 */
void control_radfan_record(can_msg_t msg);

#endif