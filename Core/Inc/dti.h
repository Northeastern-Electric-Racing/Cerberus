/**
 * @file dti.h
 * @author Hamza Iqbal + Nick DePatie
 * @brief Driver to abstract sending and receiving CAN messages to control dti motor controller
 * @version 0.1
 * @date 2023-08-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef DTI_H
#define DTI_H

#include "can_handler.h"
#include <stdbool.h>
#include <stdint.h>

/* Message IDs from DTI CAN Datasheet */
#define DTI_CANID_ERPM	      0x416 /* ERPM, Duty, Input Voltage */
#define DTI_CANID_CURRENTS    0x436 /* AC Current, DC Current */
#define DTI_CANID_ERPM	      0x416 /* ERPM, Duty, Input Voltage */
#define DTI_CANID_CURRENTS    0x436 /* AC Current, DC Current */
#define DTI_CANID_TEMPS_FAULT 0x456 /* Controller Temp, Motor Temp, Faults */
#define DTI_CANID_ID_IQ	      0x476 /* Id, Iq values */
#define DTI_CANID_SIGNALS \
	0x496 /* Throttle signal, Brake signal, IO, Drive enable */
#define DTI_QUEUE_SIZE 5

#define TIRE_DIAMETER 16 /* inches */
#define GEAR_RATIO    47 / 13.0 /* unitless */
#define POLE_PAIRS    10 /* unitless */
#define DTI_CANID_ID_IQ	      0x476 /* Id, Iq values */
#define DTI_CANID_SIGNALS \
	0x496 /* Throttle signal, Brake signal, IO, Drive enable */
#define DTI_QUEUE_SIZE 5

#define TIRE_DIAMETER 16 /* inches */
#define GEAR_RATIO    47 / 13.0 /* unitless */
#define POLE_PAIRS    10 /* unitless */

typedef struct {
	int32_t rpm; /* SCALE: 1         UNITS: Rotations per Minute   */
	int16_t duty_cycle; /* SCALE: 10        UNITS: Percentage             */
	int16_t input_voltage; /* SCALE: 1         UNITS: Volts                  */
	int16_t ac_current; /* SCALE: 10        UNITS: Amps                   */
	int16_t dc_current; /* SCALE: 10        UNITS: Amps                   */
	int16_t contr_temp; /* SCALE: 10        UNITS: Degrees Celsius        */
	int16_t motor_temp; /* SCALE: 10        UNITS: Degrees Celsius        */
	uint8_t fault_code; /* SCALE: 1         UNITS: No units just a number */
typedef struct {
	int32_t rpm; /* SCALE: 1         UNITS: Rotations per Minute   */
	int16_t duty_cycle; /* SCALE: 10        UNITS: Percentage             */
	int16_t input_voltage; /* SCALE: 1         UNITS: Volts                  */
	int16_t ac_current; /* SCALE: 10        UNITS: Amps                   */
	int16_t dc_current; /* SCALE: 10        UNITS: Amps                   */
	int16_t contr_temp; /* SCALE: 10        UNITS: Degrees Celsius        */
	int16_t motor_temp; /* SCALE: 10        UNITS: Degrees Celsius        */
	uint8_t fault_code; /* SCALE: 1         UNITS: No units just a number */
	int8_t throttle_signal; /* SCALE: 1         UNITS: Percentage             */
	int8_t brake_signal; /* SCALE: 1         UNITS: Percentage             */
	int8_t drive_enable; /* SCALE: 1         UNITS: No units just a number */
	osMutexId_t *mutex;
} dti_t;

// TODO: Expand GET interface
int32_t dti_get_rpm(dti_t *dti);

uint16_t dti_get_input_voltage(dti_t *dti);

/* Utilities for Decoding CAN message */
extern osThreadId_t dti_router_handle;
extern const osThreadAttr_t dti_router_attributes;
extern osMessageQueueId_t dti_router_queue;
void vDTIRouter(void *pv_params);

dti_t *dti_init();

/*
 * SCALE: 10
 * UNITS: Nm
 */
void dti_set_torque(int16_t torque);

/**
 * @brief Set the brake AC current target for regenerative braking.
 * 
 * @param current_target The desired AC current to do regenerative braking at.
 */
void dti_set_regen(int16_t current_target);

/*
 * SCALE: 10
 * UNITS: Amps
 */
void dti_set_brake_current(int16_t brake_current);

/*
 * SCALE: 10
 * UNITS: Percentage of max
 */
void dti_set_relative_brake_current(int16_t relative_brake_current);

/*
 * SCALE: 10
 * UNITS: Amps
 */
void dti_set_current(int16_t current);

/*
 * SCALE: 10
 * UNITS: Percentage of max
 */
void dti_set_relative_current(int16_t relative_current);

/*
 * SCALE: bool
 * UNITS: No units
 */
void dti_set_drive_enable(bool drive_enable);

#endif
