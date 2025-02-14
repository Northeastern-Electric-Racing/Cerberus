#ifndef CONTROL_H
#define CONTROL_H

#include "pdu.h"
#include "can.h"
#include "cerb_utils.h"

#define CONTROL_CANID_FANBATTBOX 0x4A1
#define CONTROL_CANID_PUMP	 0x4A0

#define MOTOR_TEMP_LIMIT 50

extern osThreadId_t control_handle;
extern const osThreadAttr_t control_attributes;

typedef struct {
	bool fanBattBoxState;
	bool pumpState0;
	bool pumpState1;
} control_t;

typedef struct {
	pdu_t *pdu;
	control_t *control;
} control_args_t;

typedef struct {
	bool state;
	control_t *control;
} set_pump_state_t;

void vControl(void *param);

void debounce_motor_temp(set_pump_state_t *set_pump, nertimer_t *pump_timer);

void set_pump_state(void *params);

void control_fanbattbox_record(control_t *control, can_msg_t msg);

void control_pump_record(control_t *control, can_msg_t msg);

#endif