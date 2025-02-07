#include <stdlib.h>

#include "control.h"
#include "cerb_utils.h"
#include "dti.h"

osThreadId_t control_handle;
const osThreadAttr_t control_attributes = {
	.name = "Control",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime,
};

void vControl(void *params)
{
	control_args_t *control_args = (control_args_t *)params;
	control_t *control = control_args->control;

	nertimer_t pumpTimer;

	set_pump_state_t *set_pump = malloc(sizeof(set_pump_state_t));
	set_pump->control = control;

	for (;;) {
		write_fan_battbox(control_args->pdu, control->fanBattBoxState);

		uint16_t motorTemp = dti_get_motor_temp();

		if (motorTemp > MOTOR_TEMP_LIMIT) {
			set_pump->state = 1;
			debounce(motorTemp > MOTOR_TEMP_LIMIT, &pumpTimer,
				 10000, &setPumpState, &set_pump);
		} else {
			set_pump->state = 0;
			debounce(motorTemp <= MOTOR_TEMP_LIMIT, &pumpTimer,
				 10000, &setPumpState, &set_pump);
		}

		write_pump_0(control_args->pdu, control->pumpState0);
		write_pump_1(control_args->pdu, control->pumpState1);

		osDelay(1000);
	}
}

void setPumpState(void *params)
{
	set_pump_state_t *set_pump_state = (set_pump_state_t *)params;
	set_pump_state->control->pumpState0 = set_pump_state->state;
	set_pump_state->control->pumpState1 = set_pump_state->state;
}

void control_fanbattbox_record(control_t *control, can_msg_t msg)
{
	control->fanBattBoxState = msg.data[0] > 0;
}

void control_pump_record(control_t *control, can_msg_t msg)
{
	control->pumpState0 = msg.data[0] > 0;
	control->pumpState1 = msg.data[1] > 0;
}
