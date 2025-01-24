#ifndef CONTROL_H
#define CONTROL_H

#include "pdu.h"
#include "can.h"

#define CONTROL_CANID_FANBATTBOX 0x4A1

extern osThreadId_t control_handle;
extern const osThreadAttr_t control_attributes;

void vControl(void *param);

void control_fanbattbox_record(can_msg_t args);

#endif