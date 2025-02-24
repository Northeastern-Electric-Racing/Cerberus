#ifndef CONTROL_H
#define CONTROL_H

#include "pdu.h"
#include "can.h"
#include "cerb_utils.h"

#define CONTROL_CANID_FANBATTBOX 0x4A1
#define CONTROL_CANID_PUMP	 0x4A0
#define CONTROL_CANID_RADFAN	 0x499

// Tempature Constants
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

typedef struct {
	bool fanBattBoxState;
	bool pumpState0;
	bool pumpState1;
	bool radfanState0;
	bool radfanState1;
} control_t;

typedef struct {
	pdu_t *pdu;
	control_t *control;
	control_t *calypso_states;
} control_args_t;

typedef struct {
	bool state;
	control_t *control;
} set_state_t;

typedef struct {
	bool *control_state;
	bool *calypso_state;
	nertimer_t timer;
	set_state_t *set_state;
	void (*set_state_func)(void *arg);
	uint16_t upper_temp;
	uint16_t lower_temp;
} device_control_t;

typedef enum { DEVICE_PUMP, DEVICE_RADFAN } device_type_t;

void vControl(void *params);

void control_device(device_type_t type, bool hv, uint16_t temp,
		    device_control_t *device);

void set_pump0_state(void *params);
void set_pump1_state(void *params);

void set_radfan0_state(void *params);
void set_radfan1_state(void *params);

void control_fanbattbox_record(control_t *calypso_states, can_msg_t msg);
void control_pump_record(control_t *calypso_states, can_msg_t msg);
void control_radfan_record(control_t *calypso_states, can_msg_t msg);

int8_t write_fan_battbox(pdu_t *pdu, bool state);

int8_t write_pump_0(pdu_t *pdu, bool state);
int8_t write_pump_1(pdu_t *pdu, bool state);

int8_t write_radfan_0(pdu_t *pdu, bool state);
int8_t write_radfan_1(pdu_t *pdu, bool state);

#endif