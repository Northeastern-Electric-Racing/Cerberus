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

void buttons_update(can_msg_t msg)
{
	uint8_t button_id = msg.data[0];

	switch (button_id) {
	case BUTTON_LEFT:
		printf("Left button pressed \n");
		left_button_cb();
		break;
	case BUTTON_RIGHT:
		printf("Right button pressed \n");
		right_button_cb();
		break;
	case BUTTON_ESC:
		printf("Esc button pressed \n");
		set_home_mode();
		break;
	case BUTTON_UP:
		printf("Up button pressed \n");
		increment_nero_index();
		break;
	case BUTTON_DOWN:
		printf("Down button pressed \n");
		decrement_nero_index();
		break;
	case BUTTON_ENTER:
		printf("Enter button pressed \n");
		select_nero_index();
		break;
	default:
		break;
	}
}

void dial_update(can_msg_t msg)
{
	uint8_t dial_switch_id = msg.data[0];

	switch (dial_switch_id) {
	case DIAL_SWITCH_1:
		printf("Dial set to switch 1 \n");
		break;
	case DIAL_SWITCH_2:
		printf("Dial set to switch 2 \n");
		break;
	case DIAL_SWITCH_3:
		printf("Dial set to switch 3 \n");
		break;
	case DIAL_SWITCH_4:
		printf("Dial set to switch 4 \n");
		break;
	case DIAL_SWITCH_5:
		printf("Dial set to switch 5 \n");
		break;
	}
}
