#include <stdlib.h>

#include "control.h"
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

	nertimer_t pump_timer;

	set_pump_state_t *set_pump = malloc(sizeof(set_pump_state_t));
	set_pump->control = control;

	for (;;) {
		// Handle Batt Box State
		write_fan_battbox(control_args->pdu, control->fanBattBoxState);

		// Handle Pump States
		bool hv;
		read_tsms_sense(control_args->pdu, &hv);

		if (hv) {
			control->pumpState0 = 1;
			control->pumpState1 = 1;
		} else {
			debounce_motor_temp(set_pump, &pump_timer);
		}

		write_pump_0(control_args->pdu, control->pumpState0);
		write_pump_1(control_args->pdu, control->pumpState1);

		osDelay(1000);
	}
}

void debounce_motor_temp(set_pump_state_t *set_pump, nertimer_t *pump_timer)
{
	// Debounce for tempature
	uint16_t motorTemp = dti_get_motor_temp();

	if (motorTemp > MOTOR_TEMP_LIMIT) {
		set_pump->state = 1;
		debounce(motorTemp > MOTOR_TEMP_LIMIT, pump_timer, 10000,
			 &set_pump_state, set_pump);
	} else {
		set_pump->state = 0;
		debounce(motorTemp <= MOTOR_TEMP_LIMIT, pump_timer, 10000,
			 &set_pump_state, set_pump);
	}
}

void set_pump_state(void *params)
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
