#include "control.h"
#include "pdu.h"

osThreadId_t control_handle;
const osThreadAttr_t control_attributes = {
	.name = "Control",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime,
};

static int fanBattBoxState = 0;

void vEval_fanbattbox_state(void *param)
{
	control_args_t args = (control_args_t *)param;
	pdu_t *pdu = args->pdu;

	write_fan_battbox(pdu, eval_fanbattbox_state());

	osDelay(1000);
}

void control_fanbattbox_record(can_msg_t msg)
{
	if (msg.data > 0) {
		fanBattBoxState = 1;
	} else {
		fanBattBoxState = 0;
	}
}

int eval_fanbattbox_state()
{
	return fanBattBoxState;
}