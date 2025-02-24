#include <stdlib.h>
#include <assert.h>

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

control_args_t *init_control(pdu_t *pdu)
{
	assert(pdu);

	control_args_t *control_args = malloc(sizeof(control_args_t));
	assert(control_args);

	control_args->pdu = pdu;
	control_args->control = malloc(sizeof(control_t));
	control_args->calypso_states = malloc(sizeof(control_t));
	control_args->control->fanBattBoxState = 0;
	control_args->control->pumpState0 = 0;
	control_args->control->pumpState1 = 0;
	control_args->calypso_states->fanBattBoxState = 0;
	control_args->calypso_states->pumpState0 = 0;
	control_args->calypso_states->pumpState1 = 0;

	return control_args;
}

void vControl(void *params)
{
	control_args_t *control_args = (control_args_t *)params;
	control_t *control = control_args->control;
	control_t *calypso_states = control_args->calypso_states;

	device_control_t pump0 = {
		.control_state = &(control->pumpState0),
		.calypso_state = &(calypso_states->pumpState0),
		.toSet = 0,
		.upper_temp = PUMP_UPPER_MOTOR_TEMP,
		.lower_temp = PUMP_LOWER_MOTOR_TEMP,
		.type = DEVICE_PUMP,
	};

	device_control_t radfan0 = {
		.control_state = &(control->radfanState0),
		.calypso_state = &(calypso_states->radfanState0),
		.toSet = 0,
		.type = DEVICE_RADFAN,
		.upper_temp = RADFAN_UPPER_MOTOR_TEMP,
		.lower_temp = RADFAN_LOWER_MOTOR_TEMP,
	};

	device_control_t pump1 = {
		.control_state = &(control->pumpState1),
		.calypso_state = &(calypso_states->pumpState1),
		.toSet = 0,
		.type = DEVICE_PUMP,
		.upper_temp = PUMP_UPPER_CONTROLLER_TEMP,
		.lower_temp = PUMP_LOWER_CONTROLLER_TEMP,
	};

	device_control_t radfan1 = {
		.control_state = &(control->radfanState1),
		.calypso_state = &(calypso_states->radfanState1),
		.toSet = 0,
		.type = DEVICE_RADFAN,
		.upper_temp = RADFAN_UPPER_CONTROLLER_TEMP,
		.lower_temp = RADFAN_LOWER_CONTROLLER_TEMP,
	};

	for (;;) {
		bool hv = get_active();
		uint16_t motor_temp = dti_get_motor_temp();
		uint16_t controller_temp = dti_get_controller_temp();

		// Determine device state
		control_device(&pump0, hv, motor_temp);
		control_device(&radfan0, hv, motor_temp);
		control_device(&pump1, hv, controller_temp);
		control_device(&radfan1, hv, controller_temp);

		// Write states
		write_fan_battbox(control_args->pdu, control->fanBattBoxState);
		write_pump_0(control_args->pdu, control->pumpState0);
		write_radfan_0(control_args->pdu, control->radfanState0);
		write_pump_1(control_args->pdu, control->pumpState1);
		write_radfan_1(control_args->pdu, control->radfanState1);

		osDelay(1000);
	}
}

void control_device(device_control_t *device, bool hv, uint16_t temp)
{
	if (device->type == DEVICE_PUMP && hv) {
		*(device->control_state) = 1;
		return;
	}

	if (temp > device->upper_temp || temp < device->lower_temp ||
	    is_timer_active(&device->timer)) {
		if (temp > device->upper_temp) {
			device->toSet = 1;
			debounce(temp > device->upper_temp, &(device->timer),
				 10000, set_device_state, device);
		} else {
			device->toSet = 0;
			debounce(temp < device->lower_temp, &(device->timer),
				 10000, set_device_state, device);
		}
	} else {
		*(device->control_state) = *(device->calypso_state);
	}
}

void set_device_state(void *params)
{
	device_control_t *device = (device_control_t *)params;
	*(device->control_state) = device->toSet;
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

int8_t write_fan_battbox(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_FANBATTBOX_CTRL, PCA_OUTPUT_0_REG);
}

int8_t write_pump_0(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_PUMP_CTRL0, PCA_OUTPUT_0_REG);
}

int8_t write_radfan_0(pdu_t *pdu, bool state)
{
	return -1; // Replace with actual stuff when PDU Radfan CTRL is added to board
}

int8_t write_pump_1(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_PUMP_CTRL1, PCA_OUTPUT_0_REG);
}

int8_t write_radfan_1(pdu_t *pdu, bool state)
{
	return -1; // Replace with actual stuff when PDU Radfan CTRL is added to board
}
