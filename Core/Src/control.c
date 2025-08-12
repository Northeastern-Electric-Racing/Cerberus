#include <assert.h>
#include <stdlib.h>

#include "control.h"
#include "state_machine.h"

bool calypso_states[NUM_DEVICES];

device_temp_bounds_t const pump0_READY = { 20, 10 };
device_temp_bounds_t const pump0_F_REVERSE = { 20, 10 };
device_temp_bounds_t const pump0_F_PIT = { 20, 10 };
device_temp_bounds_t const pump0_F_PERFORMANCE = { 20, 10 };
device_temp_bounds_t const pump0_F_EFFICIENCY = { 20, 10 };
device_temp_bounds_t const pump0_FAULTED = { 20, 10 };

device_temp_bounds_t const pump1_READY = { 20, 10 };
device_temp_bounds_t const pump1_F_REVERSE = { 20, 10 };
device_temp_bounds_t const pump1_F_PIT = { 20, 10 };
device_temp_bounds_t const pump1_F_PERFORMANCE = { 20, 10 };
device_temp_bounds_t const pump1_F_EFFICIENCY = { 20, 10 };
device_temp_bounds_t const pump1_FAULTED = { 20, 10 };

device_temp_bounds_t const radfan0_READY = { 20, 10 };
device_temp_bounds_t const radfan0_F_REVERSE = { 20, 10 };
device_temp_bounds_t const radfan0_F_PIT = { 20, 10 };
device_temp_bounds_t const radfan0_F_PERFORMANCE = { 20, 10 };
device_temp_bounds_t const radfan0_F_EFFICIENCY = { 20, 10 };
device_temp_bounds_t const radfan0_FAULTED = { 20, 10 };

device_temp_bounds_t const radfan1_READY = { 20, 10 };
device_temp_bounds_t const radfan1_F_REVERSE = { 20, 10 };
device_temp_bounds_t const radfan1_F_PIT = { 20, 10 };
device_temp_bounds_t const radfan1_F_PERFORMANCE = { 20, 10 };
device_temp_bounds_t const radfan1_F_EFFICIENCY = { 20, 10 };
device_temp_bounds_t const radfan1_FAULTED = { 20, 10 };

device_temp_bounds_t const fanBattBox_READY = { 20, 10 };
device_temp_bounds_t const fanBattBox_F_REVERSE = { 20, 10 };
device_temp_bounds_t const fanBattBox_F_PIT = { 20, 10 };
device_temp_bounds_t const fanBattBox_F_PERFORMANCE = { 20, 10 };
device_temp_bounds_t const fanBattBox_F_EFFICIENCY = { 20, 10 };
device_temp_bounds_t const fanBattBox_FAULTED = { 20, 10 };

device_config_t pump0_config = { pump0_READY,	     pump0_F_REVERSE,
				 pump0_F_PIT,	     pump0_F_PERFORMANCE,
				 pump0_F_EFFICIENCY, pump0_FAULTED };

device_config_t pump1_config = { pump1_READY,	     pump1_F_REVERSE,
				 pump1_F_PIT,	     pump1_F_PERFORMANCE,
				 pump1_F_EFFICIENCY, pump1_FAULTED };

device_config_t radfan0_config = { radfan0_READY,	 radfan0_F_REVERSE,
				   radfan0_F_PIT,	 radfan0_F_PERFORMANCE,
				   radfan0_F_EFFICIENCY, radfan0_FAULTED };

device_config_t radfan1_config = { radfan1_READY,	 radfan1_F_REVERSE,
				   radfan1_F_PIT,	 radfan1_F_PERFORMANCE,
				   radfan1_F_EFFICIENCY, radfan1_FAULTED };

device_config_t fanBattBox_config = {
	fanBattBox_READY,	  fanBattBox_F_REVERSE,	   fanBattBox_F_PIT,
	fanBattBox_F_PERFORMANCE, fanBattBox_F_EFFICIENCY, fanBattBox_FAULTED
};

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
};

// callback function to turn device off after debounce
static void set_device_off(void *params)
{
	device_control_t *device = (device_control_t *)params;
	device->control_func(device->pdu, false);
};

/**
 * @brief Sets the device state dependent on current car state and temperature.
 * 
 * @param device Device whose state is being determined.
 * @param device_bounds Temperature bounds of the device for the current car state.
 * @param temp Temperature reading to determine the state.
 */
static void set_device_state(device_control_t *device,
			     device_temp_bounds_t device_bounds, uint16_t temp)
{
	bool above_max_temp = temp > device_bounds.upper_motor_temp_bound;
	bool below_min_temp = temp < device_bounds.lower_motor_temp_bound;

	if (above_max_temp) {
		debounce(above_max_temp, &(device->timer), 10000, set_device_on,
			 device);
	} else if (below_min_temp || is_timer_active(&device->timer)) {
		debounce(below_min_temp, &(device->timer), 10000,
			 set_device_off, device);
	}
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
	if (device->device_type == DEVICE_PUMP0 ||
	    device->device_type == DEVICE_PUMP1) {
		if (hv) {
			set_device_on(device);
			return;
		} else if (get_func_state() == FAULTED) {
			set_device_off(device);
			return;
		}
	}

	// turn on device if calypso sent message to turn it on
	if (calypso_states[device->device_type]) {
		set_device_on(device);
		return;
	}

	const curr_state = get_func_state();

	// Set device state depending on car state and temperature.
	switch (curr_state) {
	case READY:
		set_device_state(device, device->temp_bounds.ready, temp);
		break;
	case F_PIT:
		set_device_state(device, device->temp_bounds.f_pit, temp);
		break;
	case F_REVERSE:
		set_device_state(device, device->temp_bounds.f_reverse, temp);
		break;
	case F_PERFORMANCE:
		set_device_state(device, device->temp_bounds.f_performance,
				 temp);
		break;
	case F_EFFICIENCY:
		set_device_state(device, device->temp_bounds.f_efficiency,
				 temp);
		break;
	case FAULTED:
		set_device_state(device, device->temp_bounds.faulted, temp);
		break;
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
		.control_func = write_pump_0,
		.temp_bounds = pump0_config,
		.device_type = DEVICE_PUMP0,
	};

	device_control_t radfan0 = {
		.pdu = pdu,
		.control_func = write_radfan_0,
		.temp_bounds = radfan0_config,
		.device_type = DEVICE_RADFAN0,
	};

	device_control_t pump1 = {
		.pdu = pdu,
		.control_func = write_pump_1,
		.temp_bounds = pump1_config,
		.device_type = DEVICE_PUMP1,
	};

	device_control_t radfan1 = {
		.pdu = pdu,
		.control_func = write_radfan_1,
		.temp_bounds = radfan1_config,
		.device_type = DEVICE_RADFAN1,
	};

	write_pump_0(pdu, false);
	write_pump_1(pdu, false);

	for (;;) {
		uint16_t motor_temp;
		dti_get_motor_temp(mc, &motor_temp);
		uint16_t controller_temp;
		dti_get_controller_temp(mc, &controller_temp);

		// Determine device state
		control_device(&pump0, motor_temp);
		control_device(&radfan0, motor_temp);
		control_device(&pump1, controller_temp);
		control_device(&radfan1, controller_temp);

		write_fan_battbox(pdu, calypso_states[DEVICE_FANBATTBOX]);

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
