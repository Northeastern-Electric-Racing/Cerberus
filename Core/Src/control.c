#include <assert.h>
#include <stdlib.h>

#include "control.h"
#include "state_machine.h"
#include <stdio.h>

#define DEVICE_ON_DEBOUNCE_TIME 1000
#define DEVICE_OFF_DEBOUNCE_TIME 15000

bool calypso_states[NUM_DEVICES];

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
 * @param temp Tempature reading to determine state
 */
static void control_device(device_control_t *device, uint16_t temp)
{
	assert(device);
	bool hv = get_active();

	// turn on pumps when hv is on / turn off when faulted
	// if (device->device_type == DEVICE_PUMP0 ||
	//     device->device_type == DEVICE_PUMP1) {
	// 	if (hv) {
	// 		set_device_on(device);
	// 		return;
	// 	} else
	if (get_func_state() == FAULTED) {
		set_device_off(device);
		return;
	}

	// uint16_t upper_temp = device->upper_temp;
	// if (!hv) {
	// 	upper_temp -= 10;
	// }

	// turn on device if calypso sent message to turn it on
	if (calypso_states[device->device_type]) {
		set_device_on(device);
		return;
	}

	// set device state based on temps with debounce
	if (temp > device->upper_temp || temp < device->lower_temp ||
	    is_timer_active(&device->timer)) {
		if (temp > device->upper_temp) {
			debounce(temp > device->upper_temp, &(device->timer),
				DEVICE_ON_DEBOUNCE_TIME, set_device_on, device);
		} else {
			debounce(temp < device->lower_temp, &(device->timer),
				DEVICE_OFF_DEBOUNCE_TIME, set_device_off, device);
		}
	}
}

void vControl(void *params)
{
	control_args_t *args = (control_args_t *)params;
	dti_t *mc = args->mc;
	assert(mc);
	pdu_t *pdu = args->pdu;
	assert(pdu);

	free(args);

	device_control_t pump0 = {
		.pdu = pdu,
		.control_func = write_pump_1,
		.upper_temp = PUMP_UPPER_MOTOR_TEMP,
		.lower_temp = PUMP_LOWER_MOTOR_TEMP,
		.device_type = DEVICE_PUMP0,
	};

	device_control_t radfan0 = {
		.pdu = pdu,
		.control_func = write_radfan_2,
		.upper_temp = RADFAN_UPPER_MOTOR_TEMP,
		.lower_temp = RADFAN_LOWER_MOTOR_TEMP,
		.device_type = DEVICE_RADFAN0,
	};

	device_control_t pump1 = {
		.pdu = pdu,
		.control_func = write_pump_2,
		.upper_temp = PUMP_UPPER_CONTROLLER_TEMP,
		.lower_temp = PUMP_LOWER_CONTROLLER_TEMP,
		.device_type = DEVICE_PUMP1,
	};

	device_control_t radfan1 = {
		.pdu = pdu,
		.control_func = write_radfan_1,
		.upper_temp = RADFAN_UPPER_CONTROLLER_TEMP,
		.lower_temp = RADFAN_LOWER_CONTROLLER_TEMP,
		.device_type = DEVICE_RADFAN1,
	};

	device_control_t fan_battbox = {
		.pdu = pdu,
		.control_func = write_fan_battbox,
		.upper_temp = FANBATTBOX_UPPER_TEMP,
		.lower_temp = FANBATTBOX_LOWER_TEMP,
		.device_type = DEVICE_FANBATTBOX,
	};

	write_pump_1(pdu, false);
	write_pump_2(pdu, false);

	for (;;) {
		uint16_t motor_temp;
		dti_get_motor_temp(mc, &motor_temp);
		uint16_t controller_temp;
		dti_get_controller_temp(mc, &controller_temp);
		uint16_t battbox_temp;
		bms_get_battbox_temp(&battbox_temp);

		if (!verify_tca_config(pdu)) {
			write_tca_config(pdu);
		}

		// Determine device state
		control_device(&pump0, motor_temp);
		control_device(&radfan0, motor_temp);
		control_device(&pump1, controller_temp);
		control_device(&radfan1, controller_temp);
		control_device(&fan_battbox, battbox_temp);

		osDelay(1000);
	}
}

void control_fanbattbox_record(can_msg_t msg)
{
	calypso_states[DEVICE_FANBATTBOX] = msg.data[0] > 0;
}

void control_pump_record(can_msg_t msg)
{
	calypso_states[DEVICE_PUMP0] = msg.data[0] > 0;
	calypso_states[DEVICE_PUMP1] = msg.data[1] > 0;
}

void control_radfan_record(can_msg_t msg)
{
	calypso_states[DEVICE_RADFAN0] = msg.data[0] > 0;
	calypso_states[DEVICE_RADFAN1] = msg.data[1] > 0;
}
