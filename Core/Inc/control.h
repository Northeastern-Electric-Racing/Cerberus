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
#include "bms.h"

#define CONTROL_CANID_FANBATTBOX 0x4A1
#define CONTROL_CANID_PUMP	 0x4A0
#define CONTROL_CANID_RADFAN	 0x499

/* Tempeature Constants for Devices */
#define PUMP_UPPER_MOTOR_TEMP	50
#define PUMP_LOWER_MOTOR_TEMP	40
#define RADFAN_UPPER_MOTOR_TEMP 50
#define RADFAN_LOWER_MOTOR_TEMP 35

#define PUMP_UPPER_CONTROLLER_TEMP   45
#define PUMP_LOWER_CONTROLLER_TEMP   35
#define RADFAN_UPPER_CONTROLLER_TEMP 45
#define RADFAN_LOWER_CONTROLLER_TEMP 35

#define FANBATTBOX_UPPER_TEMP 50
#define FANBATTBOX_LOWER_TEMP 30

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

/* Holds all the information needed to determine and set the state of a device */
typedef struct {
	pdu_t *pdu;
	control_func_t control_func; /* function to set device state */
	device_type_t device_type; /* Device Type (Pump or Radfan) */
	nertimer_t timer; /* Debounce Timer */
	uint16_t upper_temp; /* Upper Tempature Limit */
	uint16_t lower_temp; /* Lower Tempature Limit */
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