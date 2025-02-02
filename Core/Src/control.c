#include "control.h"

osThreadId_t control_handle;
const osThreadAttr_t control_attributes = {
	.name = "Control",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime,
};

static control_args_t *control_args;

void vControl(void *params)
{
	control_args = (control_args_t *)params;

	for (;;) {
		write_fan_battbox(control_args->pdu,
				  control_args->fanBattBoxState);

		write_pump_0(control_args->pdu,
			     control_args->pumpState0 &&
				     dti_get_motor_temp() <= MOTOR_TEMP_LIMIT);
		write_pump_1(control_args->pdu,
			     control_args->pumpState1 &&
				     dti_get_motor_temp() <= MOTOR_TEMP_LIMIT);

		osDelay(1000);
	}
}

void control_fanbattbox_record(can_msg_t msg)
{
	control_args->fanBattBoxState = msg.data[0] > 0;
}

void control_pump_record(can_msg_t msg)
{
	control_args->pumpState0 = msg.data[0] > 0;
	control_args->pumpState1 = msg.data[1] > 0;
}
