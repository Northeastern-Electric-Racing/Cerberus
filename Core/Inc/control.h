#ifndef CONTROL_H
#define CONTROL_H

#include "pdu.h"
#include "dti.h"
#include "can.h"

#define CONTROL_CANID_FANBATTBOX 0x4A1
#define CONTROL_CANID_PUMP	 0x4A0

#define TEMP_MOTOR_LIMIT 50

extern osThreadId_t control_handle;
extern const osThreadAttr_t control_attributes;

typedef struct {
	pdu_t *pdu;
	bool fanBattBoxState;
	bool pumpState0;
	bool pumpState1;
} control_args_t;

void vControl(void *param);

void control_fanbattbox_record(can_msg_t args);

void control_pump_record(can_msg_t msg);

#endif