#include <stdlib.h>

#include "control.h"
#include "dti.h"
#include "state_machine.h"

osThreadId_t control_handle;
const osThreadAttr_t control_attributes = {
	.name = "Control",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime,
};

void vControl(void *params)
{
	// Motor temp = RADFAN0 AND PUMP0

	// Motor Contorler temp = RADFAN0 AND PUMP0

	// PUMP0 and RADFAN0 temps not the same

	// PUMP1 and RADFAN1 temps not the same

	control_args_t *control_args = (control_args_t *)params;
	control_t *control = control_args->control;
	control_t *calypso_states = control_args->calypso_states;

	nertimer_t pump_timer0;
	nertimer_t radfan_timer0;

	nertimer_t pump_timer1;
	nertimer_t radfan_timer1;

	set_state_t *set_pump0 = malloc(sizeof(set_state_t));
	set_pump0->control = control;

	set_state_t *set_radfan0 = malloc(sizeof(set_state_t));
	set_radfan0->control = control;

	set_state_t *set_pump1 = malloc(sizeof(set_state_t));
	set_pump1->control = control;

	set_state_t *set_radfan1 = malloc(sizeof(set_state_t));
	set_radfan1->control = control;

	bool hv;

	for (;;) {
		hv = get_active();

		uint16_t motorTemp = dti_get_motor_temp();
		uint16_t motorControllerTemp = 0;
		dti_get_motor_controller_temp();

		// PUMP 0
		if (hv) {
			control->pumpState0 = 1;
		} else if (motorTemp > PUMP_UPPER_MOTOR_TEMP ||
			   motorTemp < PUMP_LOWER_MOTOR_TEMP ||
			   is_timer_active(&pump_timer0)) {
			if (motorTemp > PUMP_UPPER_MOTOR_TEMP) {
				set_pump0->state = 1;
				debounce(motorTemp > PUMP_UPPER_MOTOR_TEMP,
					 &pump_timer0, 10000, &set_pump0_state,
					 set_pump0);
			} else {
				set_pump0->state = 0;
				debounce(motorTemp < PUMP_LOWER_MOTOR_TEMP,
					 &pump_timer0, 10000, &set_pump0_state,
					 set_pump0);
			}
		} else {
			control->pumpState0 = calypso_states->pumpState0;
		}

		// RADFAN 0
		if (motorTemp > RADFAN_UPPER_MOTOR_TEMP ||
		    motorTemp < RADFAN_LOWER_MOTOR_TEMP ||
		    is_timer_active(&radfan_timer0)) {
			if (motorTemp > RADFAN_UPPER_MOTOR_TEMP) {
				set_radfan0->state = 1;
				debounce(motorTemp > RADFAN_UPPER_MOTOR_TEMP,
					 &radfan_timer0, 10000,
					 &set_radfan0_state, set_radfan0);
			} else {
				set_radfan0->state = 0;
				debounce(motorTemp < RADFAN_LOWER_MOTOR_TEMP,
					 &radfan_timer0, 10000,
					 &set_radfan0_state, set_radfan0);
			}
		} else {
			control->radfanState0 = calypso_states->radfanState0;
		}

		// PUMP 1
		if (hv) {
			control->pumpState1 = 1;
		} else if (motorControllerTemp >
				   PUMP_UPPER_MOTOR_CONTROLLER_TEMP ||
			   motorControllerTemp <
				   PUMP_LOWER_MOTOR_CONTROLLER_TEMP ||
			   is_timer_active(&pump_timer1)) {
			if (motorControllerTemp >
			    PUMP_UPPER_MOTOR_CONTROLLER_TEMP) {
				set_pump1->state = 1;
				debounce(
					motorControllerTemp >
						PUMP_UPPER_MOTOR_CONTROLLER_TEMP,
					&radfan_timer1, 10000, &set_pump1_state,
					set_pump1);
			} else {
				set_pump1->state = 0;
				debounce(
					motorControllerTemp <
						PUMP_LOWER_MOTOR_CONTROLLER_TEMP,
					&radfan_timer1, 10000, &set_pump1_state,
					set_pump1);
			}
		} else {
			control->pumpState1 = calypso_states->pumpState1;
		}

		// RADFAN 1
		if (motorControllerTemp > RADFAN_UPPER_MOTOR_CONTROLLER_TEMP ||
		    motorControllerTemp < RADFAN_LOWER_MOTOR_CONTROLLER_TEMP ||
		    is_timer_active(&radfan_timer1)) {
			if (motorControllerTemp >
			    RADFAN_UPPER_MOTOR_CONTROLLER_TEMP) {
				set_radfan1->state = 1;
				debounce(
					motorControllerTemp >
						RADFAN_UPPER_MOTOR_CONTROLLER_TEMP,
					&radfan_timer1, 10000, &set_pump1_state,
					set_radfan1);
			} else {
				set_radfan1->state = 0;
				debounce(
					motorControllerTemp <
						RADFAN_LOWER_MOTOR_CONTROLLER_TEMP,
					&radfan_timer1, 10000, &set_pump1_state,
					set_radfan1);
			}
		} else {
			control->radfanState1 = calypso_states->radfanState1;
		}

		write_fan_battbox(control_args->pdu, control->fanBattBoxState);

		write_pump_0(control_args->pdu, control->pumpState0);
		write_pump_1(control_args->pdu, control->pumpState1);

		// write_radfan_0(control->radfanState0);
		// write_radfan_1(control->radfanState1);

		osDelay(1000);
	}
}

void set_pump0_state(void *params)
{
	set_state_t *set = (set_state_t *)params;
	set->control->pumpState0 = set->state;
}

void set_pump1_state(void *params)
{
	set_state_t *set = (set_state_t *)params;
	set->control->pumpState1 = set->state;
}

void set_radfan0_state(void *params)
{
	set_state_t *set = (set_state_t *)params;
	set->control->radfanState0 = set->state;
}

void set_radfan1_state(void *params)
{
	set_state_t *set = (set_state_t *)params;
	set->control->radfanState1 = set->state;
}

void control_fanbattbox_record(control_t *calypso_states, can_msg_t msg)
{
	calypso_states->fanBattBoxState = msg.data[0] > 0;
}

void control_pump_record(control_t *calypso_states, can_msg_t msg)
{
	calypso_states->pumpState0 = msg.data[0] > 0;
	calypso_states->pumpState1 = msg.data[1] > 0;
}

void control_radfan_record(control_t *calypso_states, can_msg_t msg)
{
	calypso_states->radfanState0 = msg.data[0] > 0;
	calypso_states->radfanState1 = msg.data[1] > 0;
}
