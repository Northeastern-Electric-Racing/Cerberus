#include "nero_state_machine.h"
#include "stdint.h"
#include "stdbool.h"
#include "can_handler.h"
#include "state_machine.h"

static nero_state_t nero_state;

static void send_mode_status() {
	can_msg_t msg = { .id = 0x501, .len = 5, .data = { nero_state.home_mode, nero_state.nero_index, 0, 0, 0 } };
	/* Send CAN message */
	queue_can_msg(msg);
}

void increment_nero_index() {
	if (!nero_state.home_mode) {
		// Do Nothing because we are not in home mode and therefore not tracking the nero index
		return;
	}

	if (nero_state.nero_index + 1 < MAX_DRIVE_STATES) {
		nero_state.nero_index += 1;
		send_mode_status();
	} else {
		// Do Nothing because theres no additional states or we dont care about the additional states;
	}
}

void decrement_nero_index() {
	if (!nero_state.home_mode) {
		// Do Nothing because we are not in home mode and therefore not tracking the nero index
		return;
	}

	if (nero_state.nero_index - 1 >= 0) {
		nero_state.nero_index -= 1;
		send_mode_status();
	} else {
		// Do Nothing because theres no negative states
	}
}

void select_nero_index() {
	if (!nero_state.home_mode) {
		// Do Nothing because we are not in home mode and therefore not tracking the nero index
		return;
	}

	if (nero_state.nero_index >= 0 && nero_state.nero_index < MAX_DRIVE_STATES) {
		nero_state.home_mode = false;
		send_mode_status();
		queue_drive_state(nero_state.nero_index);
	} else {
		// Do Nothing because the index is out of bounds
	}
}

void set_home_mode() {
	nero_state.home_mode = true;
	send_mode_status();
}
