#include "steeringio.h"

#include <stdio.h>

#include "pedals.h"
#include "state_machine.h"

#define CAN_QUEUE_SIZE 5 /* messages */

void steeringio_update(can_msg_t msg)
{
	uint8_t button_id = msg.data[0];

	switch (button_id) {
	case BUTTON_LEFT:
		printf("Left button pressed \n");
		if (get_func_state() == F_EFFICIENCY) {
			decrease_torque_limit();
		}
		decrement_nero_index();
		break;
	case BUTTON_RIGHT:
		printf("Right button pressed \n");
		if (get_func_state() == F_EFFICIENCY) {
			increase_torque_limit();
		}
		increment_nero_index();
		break;
	case BUTTON_ESC:
		printf("Esc button pressed \n");
		set_home_mode();
		break;
	case BUTTON_UP:
		printf("Up button pressed \n");
		decrement_nero_index();
		break;
	case BUTTON_DOWN:
		printf("Down button pressed \n");
		increment_nero_index();
		break;
	case BUTTON_ENTER:
		printf("Enter button pressed \n");
		select_nero_index();
		break;
	default:
		break;
	}
}
