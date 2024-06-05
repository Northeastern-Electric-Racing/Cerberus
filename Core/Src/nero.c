#include "nero.h"
#include "stdint.h"
#include "stdbool.h"
#include "can_handler.h"
#include "state_machine.h"
#include "serial_monitor.h"
#include "queues.h"
#include "c_utils.h"
#include "stdio.h"
#include "cerberus_conf.h"
#include "monitor.h"

static nero_state_t nero_state = { .nero_index = 0, .home_mode = true};
static int8_t mph = 0;

static void send_mode_status() {
	can_msg_t msg = { .id = 0x501, .len = 4, .data = { nero_state.home_mode, nero_state.nero_index, mph, get_tsms() } };

	/* Send CAN message */
	queue_can_msg(msg);
}

static drive_state_t map_nero_index_to_drive_state(nero_menu_t nero_index) {
	switch (nero_index) {
		case OFF:
			return NOT_DRIVING;
		case PIT:
			return SPEED_LIMITED;
		case PERFORMANCE:
			return AUTOCROSS;
		case EFFICIENCY:
			return ENDURANCE;
		default:
			return OFF;
	}
}

void increment_nero_index() {
	if (!nero_state.home_mode) {
		// Do Nothing because we are not in home mode and therefore not tracking the nero index
		return;
	}

	if (nero_state.nero_index + 1 < MAX_NERO_STATES) {
		nero_state.nero_index += 1;
	} else {
		// Do Nothing because theres no additional states or we dont care about the additional states;
	}
}

void set_mph(int8_t new_mph) {
	//printf("mph %d", new_mph);
	mph = new_mph;
}

void decrement_nero_index() {
	serial_print("decrementing with mode, %d", nero_state.home_mode);
	if (!nero_state.home_mode) {
		// Do Nothing because we are not in home mode and therefore not tracking the nero index
		return;
	}

	if (nero_state.nero_index - 1 >= 0) {
		nero_state.nero_index -= 1;
	} else {
		// Do Nothing because theres no negative states
	}
}

void select_nero_index() {
	state_req_t state_request;

	if (!nero_state.home_mode) {
		// Only Check if we are in pit mode to toggle direction
		if (nero_state.nero_index == PIT) {
			if (get_drive_state() == REVERSE) {
				state_request.id = DRIVE;
				state_request.state.drive = SPEED_LIMITED;
				queue_state_transition(state_request);
			} else {
				state_request.id = DRIVE;
				state_request.state.drive = REVERSE;
				queue_state_transition(state_request);
			}
		}
		return;
	}

	uint8_t max_drive_states = MAX_DRIVE_STATES - 1; // Account for reverse and pit being the same screen

	if (nero_state.nero_index > 0 && nero_state.nero_index < max_drive_states && get_tsms() && get_brake_state()) {
		state_request.id = FUNCTIONAL;
		state_request.state.functional = ACTIVE;
		queue_state_transition(state_request);
		state_request.id = DRIVE;
		state_request.state.drive = map_nero_index_to_drive_state(nero_state.nero_index);
		queue_state_transition(state_request);
		nero_state.home_mode = false;
	} else if (nero_state.nero_index == OFF) {
		state_request.id = FUNCTIONAL;
		state_request.state.functional = READY;
		queue_state_transition(state_request);
		nero_state.home_mode = false;
	} else if (nero_state.nero_index >= max_drive_states) {
		nero_state.home_mode = false;
	}
}

void set_home_mode() {
	state_req_t state_request = {.id = FUNCTIONAL, .state.functional = READY};
	if (!get_tsms()) {
		nero_state.home_mode = true;
		queue_state_transition(state_request);
	}
}

osThreadId_t nero_monitor_handle;
const osThreadAttr_t nero_monitor_attributes = {
	.name		= "NeroMonitor",
	.stack_size = 32 * 8,
	.priority	= (osPriority_t)osPriorityHigh2,
};

void vNeroMonitor(void* pv_params) {
	for (;;) {
		send_mode_status();

		osDelay(NERO_DELAY_TIME);
	}
}