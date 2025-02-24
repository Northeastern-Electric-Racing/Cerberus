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
	control_args_t *control_args = (control_args_t *)params;
	control_t *control = control_args->control;
	control_t *calypso_states = control_args->calypso_states;

	// Pump 0 Structure
	device_control_t pump0 = { .control_state = &(control->pumpState0),
				   .calypso_state =
					   &(calypso_states->pumpState0),
				   .set_state_func = set_pump0_state,
				   .upper_temp = PUMP_UPPER_MOTOR_TEMP,
				   .lower_temp = PUMP_LOWER_MOTOR_TEMP };
	pump0.set_state = malloc(sizeof(set_state_t));
	pump0.set_state->control = control;

	// Radfan 0 Structure
	device_control_t radfan0 = { .control_state = &(control->radfanState0),
				     .calypso_state =
					     &(calypso_states->radfanState0),
				     .set_state_func = set_radfan0_state,
				     .upper_temp = RADFAN_UPPER_MOTOR_TEMP,
				     .lower_temp = RADFAN_LOWER_MOTOR_TEMP };
	radfan0.set_state = malloc(sizeof(set_state_t));
	radfan0.set_state->control = control;

	// Pump 1 Structure
	device_control_t pump1 = { .control_state = &(control->pumpState1),
				   .calypso_state =
					   &(calypso_states->pumpState1),
				   .set_state_func = set_pump1_state,
				   .upper_temp = PUMP_UPPER_CONTROLLER_TEMP,
				   .lower_temp = PUMP_LOWER_CONTROLLER_TEMP };
	pump1.set_state = malloc(sizeof(set_state_t));
	pump1.set_state->control = control;

	// Radfan 1 Structure
	device_control_t radfan1 = {
		.control_state = &(control->radfanState1),
		.calypso_state = &(calypso_states->radfanState1),
		.set_state_func = set_radfan1_state,
		.upper_temp = RADFAN_UPPER_CONTROLLER_TEMP,
		.lower_temp = RADFAN_LOWER_CONTROLLER_TEMP
	};
	radfan1.set_state = malloc(sizeof(set_state_t));
	radfan1.set_state->control = control;

	for (;;) {
		bool hv = get_active();
		uint16_t motor_temp = dti_get_motor_temp();
		uint16_t controller_temp = dti_get_controller_temp();

		// Control devices
		control_device(DEVICE_PUMP, hv, motor_temp, &pump0);
		control_device(DEVICE_RADFAN, hv, motor_temp, &radfan0);
		control_device(DEVICE_PUMP, hv, controller_temp, &pump1);
		control_device(DEVICE_RADFAN, hv, controller_temp, &radfan1);

		// Write states
		write_fan_battbox(control_args->pdu, control->fanBattBoxState);
		write_pump_0(control_args->pdu, control->pumpState0);
		write_pump_1(control_args->pdu, control->pumpState1);
		write_radfan_0(control_args->pdu, control->radfanState0);
		write_radfan_1(control_args->pdu, control->radfanState1);

		osDelay(1000);
	}
}

// Control logic for devices
void control_device(device_type_t type, bool hv, uint16_t temp,
		    device_control_t *device)
{
	if (type == DEVICE_PUMP && hv) {
		*(device->control_state) = 1;
		return;
	}

	if (temp > device->upper_temp || temp < device->lower_temp ||
	    is_timer_active(&device->timer)) {
		if (temp > device->upper_temp) {
			device->set_state->state = 1;
			debounce(temp > device->upper_temp, &(device->timer),
				 10000, device->set_state_func,
				 device->set_state);
		} else {
			device->set_state->state = 0;
			debounce(temp < device->lower_temp, &(device->timer),
				 10000, device->set_state_func,
				 device->set_state);
		}
	} else {
		*(device->control_state) = *(device->calypso_state);
	}
}

// Setting state from debounce
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

// Recording CAN message state
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

// writing to devices
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
