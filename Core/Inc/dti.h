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
#define DTI_CANID_ERPM		  0x416 /* ERPM, Duty, Input Voltage */
#define DTI_CANID_CURRENTS	  0x436 /* AC Current, DC Current */
#define DTI_CANID_TEMPS_FAULT 0x456 /* Controller Temp, Motor Temp, Faults */
#define DTI_CANID_ID_IQ		  0x476 /* Id, Iq values */
#define DTI_CANID_SIGNALS	  0x496 /* Throttle signal, Brake signal, IO, Drive enable */

/* DTI OPERATING LIMITS */
/* Note these may have to be adjusted (Hamza's rough guesses) */
#define MAX_RPM 			  6500
#define MAX_DUTY			  1000
#define MAX_INPUT_VOLTAGE	  6500
#define MIN_INPUT_VOLTAGE	  5500
#define MAX_DC_CURRENT		  3300
#define MIN_DC_CURRENT		  -3300
#define MAX_AC_CURRENT		  3300
#define MIN_AC_CURRENT		  -3300
#define CTRL_TEMP_LIMIT		  500
#define MOTOR_TEMP_LIMIT	  500
#define MAX_FOC_ID			  300000
#define MAX_FOC_IQ			  300000


/* DTI ERROR CODES */
/* Note treating most of these as critical errors for now */
#define DTI_NO_FAULTS		  0x00
#define DTI_OVERVOLTAGE		  0x01
#define DTI_UNDERVOLTAGE	  0x02
#define DTI_DRV			      0x03
#define DTI_ABS_OVERCURRENT	  0x04
#define DTI_CTLR_OVERTEMP	  0x05
#define DTI_MOTOR_OVERTEMP	  0x06
#define DTI_SENSOR_WIRE_FAULT 0x07
#define DTI_SENSOR_GEN_FAULT  0x08
#define DTI_CAN_CMD_ERR		  0x09
#define DTI_ANLG_IN_ERR       0x0A

typedef struct
{
	uint8_t digital_in_1 : 1;
    uint8_t digital_in_2 : 1;
    uint8_t digital_in_3 : 1;
    uint8_t digital_in_4 : 1;
    uint8_t digital_out_1 : 1;
    uint8_t digital_out_2 : 1;
    uint8_t digital_out_3 : 1;
    uint8_t digital_out_4 : 1;
    uint8_t cap_temp_limit : 1;
    uint8_t dc_current_limit : 1;
    uint8_t drive_enable_limit : 1;
    uint8_t igbt_acc_temp_limit : 1;
    uint8_t igbt_temp_limit : 1;
    uint8_t input_voltage_limit : 1;
    uint8_t motor_acc_temp_limit : 1;
    uint8_t motor_temp_limit : 1;
    uint8_t rpm_min_limit : 1;
    uint8_t rpm_max_limit : 1;
    uint8_t power_limit : 1;
    uint8_t can_map_version : 1;
} dti_signals_t;

typedef struct 
{
	int32_t rpm;			/* SCALE: 1         UNITS: Rotations per Minute   */
	int16_t duty_cycle;		/* SCALE: 10        UNITS: Percentage             */
	int16_t input_voltage;	/* SCALE: 1         UNITS: Volts                  */
	int16_t ac_current;		/* SCALE: 10        UNITS: Amps                   */
	int16_t dc_current;		/* SCALE: 10        UNITS: Amps                   */
	int16_t contr_temp;		/* SCALE: 10        UNITS: Degrees Celsius        */
	int16_t motor_temp;		/* SCALE: 10        UNITS: Degrees Celsius        */
	uint8_t fault_code;		/* SCALE: 1         UNITS: No units just a number */
	int32_t foc_id;			/* SCALE: 100		UNITS: Amps					  */	
	int32_t foc_iq;			/* SCALE: 100		UNITS: Amps					  */
	int8_t throttle_signal; /* SCALE: 1         UNITS: Percentage             */
	int8_t brake_signal;	/* SCALE: 1         UNITS: Percentage             */
	int8_t drive_enable;	/* SCALE: 1         UNITS: No units just a number */
	dti_signals_t signals;	/* Bitfield of signal status flags                */
	osMutexId_t* mutex;
} dti_t;

// TODO: Expand GET interface

/* Utilities for Decoding CAN message */
extern osThreadId_t dti_router_handle;
extern const osThreadAttr_t dti_router_attributes;
extern osMessageQueueId_t dti_router_queue;
void vDTIRouter(void* pv_params);

/*Functions to Decode and Handle CAN messages*/
//typedef void (*DTI_Message_Handler)(const can_msg_t *, dti_t *);
void handle_ERPM(const can_msg_t *msg, dti_t *dti);
void handle_CURRENTS(const can_msg_t *msg, dti_t *dti);
void handle_TEMPS_FAULTS(const can_msg_t *msg, dti_t *dti);
void handle_ID_IQ(const can_msg_t *msg, dti_t *dti);
void handle_SIGNALS(const can_msg_t *msg, dti_t *dti);

// DTI_Message_Handler DTI_Handlers[5] =
// [
// 	handle_ERPM,
// 	handle_CURRENTS,
// 	handle_TEMPS_FAULTS,
// 	handle_ID_IQ,
// 	handle_SIGNALS
// ];


dti_t* dti_init();

/*
 * SCALE: 10
 * UNITS: Nm
 */
void dti_set_torque(int16_t torque);

// TODO: Regen interface
// void dti_set_regen(int16_t regen);

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
