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

#include "pdu.h"
#include "can.h"
#include "debounce.h"

#define CONTROL_CANID_FANBATTBOX 0x4A1
#define CONTROL_CANID_PUMP	 0x4A0
#define CONTROL_CANID_RADFAN	 0x499

/* Tempeature Constants for Devices */
#define PUMP_UPPER_MOTOR_TEMP 50
#define PUMP_LOWER_MOTOR_TEMP 30

#define RADFAN_UPPER_MOTOR_TEMP 50
#define RADFAN_LOWER_MOTOR_TEMP 30

#define PUMP_UPPER_CONTROLLER_TEMP 50
#define PUMP_LOWER_CONTROLLER_TEMP 30

#define RADFAN_UPPER_CONTROLLER_TEMP 50
#define RADFAN_LOWER_CONTROLLER_TEMP 30

extern osThreadId_t control_handle;
extern const osThreadAttr_t control_attributes;

typedef int8_t (*control_func_t)(pdu_t *pdu, bool state);

typedef enum { DEVICE_PUMP, DEVICE_RADFAN } device_type_t;

/* Holds the state information for all devices */
typedef struct {
	bool fanBattBoxState;
	bool pumpState0;
	bool pumpState1;
	bool radfanState0;
	bool radfanState1;
} control_t;

/* Information given when thread initializes */
typedef struct {
	pdu_t *pdu;
	control_t *control; /* True states of device */
	control_t *calypso_states; /* States calypso wants to set the device to */
} control_args_t;

/* Holds all the information needed to determine and set the state of a device */
typedef struct {
	pdu_t *pdu;
	bool control_state; /* True state of device */
	bool calypso_state; /* The state calypso wants to set the device to */
	control_func_t control_func;
	device_type_t type; /* Device Type (Pump or Radfan) */
	nertimer_t timer; /* Debounce Timer */
	uint16_t upper_temp; /* Upper Tempature Limit */
	uint16_t lower_temp; /* Lower Tempature Limit */
} device_control_t;

/**
 * @brief Initializes control_args_t struct
 * 
 * @param pdu Pointer pdu_t struct
 * @return control_args_t* Pointer to Control Arguments struct
 */
control_args_t *control_init(pdu_t *pdu);

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
void control_fanbattbox_record(control_t *calypso_states, can_msg_t msg);

/**
 * @brief Records pump0 and pump1 states sent through CAN
 * 
 * @param calypso_states Pointer calypso states struct
 * @param msg CAN message sent
 */
void control_pump_record(control_t *calypso_states, can_msg_t msg);

/**
 * @brief Records radfan1 and radfan2 states sent through CAN
 * 
 * @param calypso_states Pointer calypso states struct
 * @param msg CAN message sent
 */
void control_radfan_record(control_t *calypso_states, can_msg_t msg);

#endif