#include "steeringio.h"

#include <stdio.h>

#include "pedals.h"
#include "state_machine.h"

#define CAN_QUEUE_SIZE 5 /* messages */

static void left_button_cb()
{
	if (get_func_state() == F_EFFICIENCY) {
		increase_torque_limit();
	}
}

static void right_button_cb()
{
	if (get_func_state() == F_EFFICIENCY) {
		decrease_torque_limit();
	}
}

void steeringio_update(can_msg_t msg)
{
	uint8_t button_id = msg.data[0];

	switch (button_id) {
	case NERO_BUTTON_UP:
		printf("Up button pressed \n");
		decrement_nero_index();
		break;
	case NERO_BUTTON_DOWN:
		printf("Down button pressed \n");
		increment_nero_index();
		break;
	case NERO_BUTTON_LEFT:
		left_button_cb();
		break;
	case NERO_BUTTON_RIGHT:
		right_button_cb();
		break;
	case NERO_BUTTON_SELECT:
		printf("Select button pressed \n");
		select_nero_index();
		break;
	case NERO_HOME:
		printf("Home button pressed \n");
		set_home_mode();
		break;
	default:
		break;
	}
}
