#include "control.h"
#include "pdu.h"

osThreadId_t control_handle;
const osThreadAttr_t control_attributes = {
	.name = "Control",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime,
};

static int fanBattBoxState = 0;

void vControl(void *param)
{
	pdu_t *pdu = (pdu_t *)param;

	for (;;) {
		write_fan_battbox(pdu, fanBattBoxState);

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
