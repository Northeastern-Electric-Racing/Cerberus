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
	int8_t throttle_signal; /* SCALE: 1         UNITS: Percentage             */
	int8_t brake_signal; /* SCALE: 1         UNITS: Percentage             */
	int8_t drive_enable; /* SCALE: 1         UNITS: No units just a number */
	osMutexId_t *mutex;
} dti_t;

/**
 * @brief Initialize DTI interface.
 * 
 * @return dti_t* Pointer to DTI struct
 */
dti_t *dti_init();

/**
 * @brief Get the RPM of the motor.
 * 
 * @param dti Pointer to DTI struct
 * @return int32_t The RPM of the motor
 */
int32_t dti_get_rpm(dti_t *dti);

/**
 * @brief Process DTI ERPM CAN message.
 * 
 * @param mc Pointer to struct representing motor controller
 * @param msg CAN message to process
 */
void dti_record_rpm(dti_t *mc, can_msg_t msg);

/**
 * @brief Get the MPH of the motor.
 * 
 * @param dti Pointer to DTI struct
 * @return float 
 */
float dti_get_mph(dti_t *dti);

/**
 * @brief Get the input voltage of the DTI.
 * 
 * @param dti Pointer to DTI struct
 * @return uint16_t Input voltage of the DTI
 */
uint16_t dti_get_input_voltage(dti_t *dti);

/**
 * @brief Send CAN message to command torque from the motor controller. The torque to command is smoothed with a moving average before being send to the motor controller.
 * 
 * @param torque The torque target.
 */
void dti_set_torque(int16_t torque);

/**
 * @brief Set the brake AC current target for regenerative braking. Only positive values are accepted by the DTI.
 * 
 * @param current_target The desired AC current to do regenerative braking at. Must be positive. This argument must be the actual value to set multiplied by 10.
 */
void dti_set_regen(uint16_t current_target);

/**
 * @brief Send a CAN message containing the AC current target for regenerative braking.
 * 
 * @param brake_current AC current target for regenerative braking. The actual value sent to the motor controller must be multiplied by 10.
 */
void dti_send_brake_current(uint16_t brake_current);

/**
 * @brief Send message for relative brake current target.
 * 
 * @param relative_brake_current Percentage of brake current maximum multiplied by 10
 */
void dti_set_relative_brake_current(int16_t relative_brake_current);

/**
 * @brief Send AC current target command to DTI.
 * 
 * @param current AC current target multiplied by 10
 */
void dti_set_current(int16_t current);

/**
 * @brief Send relative AC current target command to DTI.
 * 
 * @param relative_current Percent of the maximum AC current multiplied by 10
 */
void dti_set_relative_current(int16_t relative_current);

/**
 * @brief Send drive enable command to DTI.
 * 
 * @param drive_enable True to enable driving, false to disable
 */
void dti_set_drive_enable(bool drive_enable);

#endif
