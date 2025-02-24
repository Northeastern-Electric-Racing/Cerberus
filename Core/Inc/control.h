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
#include "cerb_utils.h"

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
	control_t *control;
	control_t *calypso_states;
} control_args_t;

/* Holds all the information needed to determine and set the state of a device */
typedef struct {
	bool *control_state; /* True state of device */
	bool *calypso_state; /* The state calypso wants to set the device to */
	bool toSet; /* The state debounce wants to set the device to */
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
 * @brief Determines and sets the state of the given device
 * 
 * @param device Device whose state is being determined
 * @param hv High voltage or not
 * @param temp Tempature reading to determine state 
 */
void control_device(device_control_t *device, bool hv, uint16_t temp);

/**
 * @brief Sets the device state determined by debounce
 * 
 * @param params Pointer to device_control_t struct
 */
void set_device_state(void *params);

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

/**
 * @brief Writes the given state into the fan battbox
 * 
 * @param pdu Pointer to pdu struct
 * @param state State being written to fan battbox
 * @return 8 bit integer
 */
int8_t write_fan_battbox(pdu_t *pdu, bool state);

/**
 * @brief Writes the given state into pump0
 * 
 * @param pdu Pointer to pdu struct
 * @param state State being written to pump0
 * @return 8 bit integer
 */
int8_t write_pump_0(pdu_t *pdu, bool state);

/**
 * @brief Writes the given state into radfan0
 * 
 * @param pdu Pointer to pdu struct
 * @param state State being written to radfan0
 * @return 8 bit integer
 */
int8_t write_radfan_0(pdu_t *pdu, bool state);

/**
 * @brief Writes the given state into pump1
 * 
 * @param pdu Pointer to pdu struct
 * @param state State being written to pump1
 * @return 8 bit integer
 */
int8_t write_pump_1(pdu_t *pdu, bool state);

/**
 * @brief Writes the given state into radfan1
 * 
 * @param pdu Pointer to pdu struct
 * @param state State being written to radfan1
 * @return 8 bit integer
 */
int8_t write_radfan_1(pdu_t *pdu, bool state);

#endif