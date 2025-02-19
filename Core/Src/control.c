#include <stdlib.h>

#include "control.h"
#include "dti.h"
#include "pdu.h"
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
	// Motor Controler temp = RADFAN0 AND PUMP0
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
		uint16_t motorControllerTemp = dti_get_motor_controller_temp();
		dti_get_motor_controller_temp();

		// PUMP 0
		control_pump(hv, motorTemp, PUMP_UPPER_MOTOR_TEMP,
			     PUMP_LOWER_MOTOR_TEMP, &pump_timer0, set_pump0,
			     &set_pump0_state, &contorl->pumpState0,
			     &calypso_states->pumpState0);

		// RADFAN 0
		control_radfan(motorTemp, RADFAN_UPPER_MOTOR_TEMP,
			       RADFAN_LOWER_MOTOR_TEMP, &radfan_timer0,
			       set_radfan0, &set_radfan0_state,
			       &control->radfanState0,
			       &calypso_states->radfanState0);

		// PUMP 1
		control_pump(hv, motorControllerTemp,
			     PUMP_UPPER_MOTOR_CONTROLLER_TEMP,
			     PUMP_LOWER_MOTOR_CONTROLLER_TEMP, &pump_timer1,
			     set_pump1, &set_pump1_state, &control->pumpState1,
			     &calypso_states->pumpState1);

		// RADFAN 1
		control_radfan(motorControllerTemp,
			       RADFAN_UPPER_MOTOR_CONTROLLER_TEMP,
			       RADFAN_LOWER_MOTOR_CONTROLLER_TEMP,
			       &radfan_timer1, set_radfan1, &set_radfan1_state,
			       &control->radfanState1,
			       &calypso_states->radfanState1);

		write_fan_battbox(control_args->pdu, control->fanBattBoxState);

		write_pump_0(control_args->pdu, control->pumpState0);
		write_pump_1(control_args->pdu, control->pumpState1);

		write_radfan_0(control->radfanState0);
		write_radfan_1(control->radfanState1);

		osDelay(1000);
	}
}

void control_pump(bool hv, uint16_t temp, uint16_t upper, uint16_t lower,
		  nertimer_t *timer, set_state_t *set_state,
		  void (*func)(void *arg), bool *control_state,
		  bool *calypso_state)
{
	if (hv) {
		(*control_state) = 1;
	} else if (temp > upper || temp < lower || is_timer_active(timer)) {
		if (temp > upper) {
			set_state->state = 1;
			debounce(temp > upper, timer, 10000, func, set_state);
		} else {
			set_state->state = 0;
			debounce(temp < lower, timer, 10000, func, set_state);
		}
	} else {
		(*control_state) = (*calypso_state);
	}
}

void control_radfan(uint16_t temp, uint16_t upper, uint16_t lower,
		    nertimer_t *timer, set_state_t *set_state,
		    void (*func)(void *arg), bool *control_state,
		    bool *calypso_state)
{
	if (temp > upper || temp < lower || is_timer_active(timer)) {
		if (temp > upper) {
			set_state->state = 1;
			debounce(temp > upper, timer, 10000, func, set_state);
		} else {
			set_state->state = 0;
			debounce(temp < lower, timer, 10000, func, set_state);
		}
	} else {
		(*control_state) = (*calypso_state);
	}
}

// Setting State From Debounce Functions
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

// Recording CAN Message State
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

// Writing To Devices
int8_t write_fan_battbox(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_FANBATTBOX_CTRL, PCA_OUTPUT_0_REG);
}

int8_t write_pump_0(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_PUMP_CTRL0, PCA_OUTPUT_0_REG);
}

int8_t write_pump_1(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_PUMP_CTRL1, PCA_OUTPUT_0_REG);
}

int8_t write_radfan_0(pdu_t *pdu, bool state)
{
	return -1; // Replace with actual stuff when PDU Radfan CTRL is added to board
}

int8_t write_radfan_1(pdu_t *pdu, bool state)
{
	return -1; // Replace with actual stuff when PDU Radfan CTRL is added to board
}
