#include "control.h"
#include "pdu.h"

osThreadId_t control_handle;
const osThreadAttr_t control_attributes = {
	.name = "Control",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime,
};

static int fanBattBoxState = 0;

static int pumpState0 = 0;

static int pumpState1 = 0;

void vControl(void *param)
{
	pdu_t *pdu = (pdu_t *)param;

	for (;;) {
		write_fan_battbox(pdu, fanBattBoxState);

		write_pump_0(pdu, pumpState0);
		write_pump_1(pdu, pumpState1);

		osDelay(1000);
	}
}

void control_fanbattbox_record(can_msg_t msg)
{
	if (msg.data[0] > 0) {
		fanBattBoxState = 1;
	} else {
		fanBattBoxState = 0;
	}
}

void control_pump_record(can_msg_t msg)
{
	if (msg.data[0] > 0) {
		pumpState0 = 1;
	} else {
		pumpState0 = 0;
	}

	if (msg.data[1] > 0) {
		pumpState1 = 1;
	} else {
		pumpState1 = 0;
	}
}
