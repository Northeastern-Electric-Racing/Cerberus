#include <stdlib.h>
#include <assert.h>

#include "control.h"
#include "dti.h"
#include "state_machine.h"

osThreadId_t control_handle;
const osThreadAttr_t control_attributes = {
	.name = "Control",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityHigh,
};

// callback function to turn device on after debounce
static void set_device_on(void *params)
{
	device_control_t *device = (device_control_t *)params;
	device->control_func(device->pdu, true);
}

// callback function to turn device off after debounce
static void set_device_off(void *params)
{
	device_control_t *device = (device_control_t *)params;
	device->control_func(device->pdu, false);
}

/**
 * @brief Determines and sets the state of the given device
 * 
 * @param device Device whose state is being determined
 * @param hv High voltage or not
 * @param temp Tempature reading to determine state 
 */
static void control_device(device_control_t *device, uint16_t temp)
{
	assert(device);

	bool hv = get_active();
	if (device->type == DEVICE_PUMP && hv) {
		device->control_state = 1;
		return;
	}

	if (temp > device->upper_temp || temp < device->lower_temp ||
	    is_timer_active(&device->timer)) {
		if (temp > device->upper_temp) {
			debounce(temp > device->upper_temp, &(device->timer),
				 10000, set_device_on, device);
		} else {
			debounce(temp < device->lower_temp, &(device->timer),
				 10000, set_device_off, device);
		}
	} else {
		device->control_state = device->calypso_state;
	}
}

control_args_t *control_init(pdu_t *pdu)
{
	assert(pdu);

	control_args_t *control_args = malloc(sizeof(control_args_t));
	assert(control_args);

	control_args->pdu = pdu;

	control_args->control = malloc(sizeof(control_t));
	assert(control_args->control);

	control_args->calypso_states = malloc(sizeof(control_t));
	assert(control_args->calypso_states);

	control_args->control->fanBattBoxState = 0;
	control_args->control->pumpState0 = 0;
	control_args->control->pumpState1 = 0;
	control_args->control->radfanState0 = 0;
	control_args->control->radfanState1 = 0;

	control_args->calypso_states->fanBattBoxState = 0;
	control_args->calypso_states->pumpState0 = 0;
	control_args->calypso_states->pumpState1 = 0;
	control_args->calypso_states->radfanState0 = 0;
	control_args->calypso_states->radfanState1 = 0;

	return control_args;
}

void vControl(void *params)
{
	control_args_t *control_args = (control_args_t *)params;
	pdu_t *pdu = control_args->pdu;
	control_t *control = control_args->control;
	control_t *calypso_states = control_args->calypso_states;

	device_control_t pump0 = {
		.pdu = pdu,
		.control_state = control->pumpState0,
		.calypso_state = calypso_states->pumpState0,
		.control_func = write_pump_0,
		.upper_temp = PUMP_UPPER_MOTOR_TEMP,
		.lower_temp = PUMP_LOWER_MOTOR_TEMP,
		.type = DEVICE_PUMP,
	};

	device_control_t radfan0 = {
		.pdu = pdu,
		.control_state = &(control->radfanState0),
		.calypso_state = &(calypso_states->radfanState0),
		.control_func = write_radfan_0,
		.type = DEVICE_RADFAN,
		.upper_temp = RADFAN_UPPER_MOTOR_TEMP,
		.lower_temp = RADFAN_LOWER_MOTOR_TEMP,
	};

	device_control_t pump1 = {
		.pdu = pdu,
		.control_state = &(control->pumpState1),
		.calypso_state = &(calypso_states->pumpState1),
		.control_func = write_pump_1,
		.type = DEVICE_PUMP,
		.upper_temp = PUMP_UPPER_CONTROLLER_TEMP,
		.lower_temp = PUMP_LOWER_CONTROLLER_TEMP,
	};

	device_control_t radfan1 = {
		.pdu = pdu,
		.control_state = &(control->radfanState1),
		.calypso_state = &(calypso_states->radfanState1),
		.control_func = write_radfan_1,
		.type = DEVICE_RADFAN,
		.upper_temp = RADFAN_UPPER_CONTROLLER_TEMP,
		.lower_temp = RADFAN_LOWER_CONTROLLER_TEMP,
	};

	write_pump_0(pdu, false);
	write_pump_1(pdu, false);

	for (;;) {
		uint16_t motor_temp = dti_get_motor_temp();
		uint16_t controller_temp = dti_get_controller_temp();

		// Determine device state
		control_device(&pump0, motor_temp);
		control_device(&radfan0, motor_temp);
		control_device(&pump1, controller_temp);
		control_device(&radfan1, controller_temp);

		osDelay(1000);
	}
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
