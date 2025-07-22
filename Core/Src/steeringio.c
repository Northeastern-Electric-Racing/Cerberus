#include "steeringio.h"

#include <stdio.h>

#include "pedals.h"
#include "state_machine.h"

#define CAN_QUEUE_SIZE 5 /* messages */

static void set_torque_limit_wrapper(float percentage)
{
	if (get_func_state() == F_EFFICIENCY) {
		set_torque_limit(percentage);
	}
}

void buttons_update(can_msg_t msg, dti_t *mc)
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
		if (fabs(dti_get_mph(mc)) < 1) {
			set_home_mode();
		}
		break;
	case BUTTON_UP:
		printf("Up button pressed \n");
		if (get_func_state() == F_EFFICIENCY ||
		    (dti_get_mph(mc) <= 0 && get_brake_state() &&
		     get_func_state() == F_PERFORMANCE)) {
			increase_regen_limit();
		}
		decrement_nero_index();
		break;
	case BUTTON_DOWN:
		printf("Down button pressed \n");
		if (get_func_state() == F_EFFICIENCY ||
		    (dti_get_mph(mc) <= 0 && get_brake_state() &&
		     get_func_state() == F_PERFORMANCE)) {
			decrease_regen_limit();
		}
		increment_nero_index();
		break;
	case BUTTON_ENTER:
		printf("Enter button pressed \n");
		if (get_func_state() == F_PERFORMANCE &&
		    fabs(dti_get_mph(mc)) < 1) {
			toggle_launch_control();
		}
		select_nero_index();
		break;
	case BUTTON_SPARE:
		printf("Spare button pressed \n");
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
		set_torque_limit_wrapper(0.2);
		break;
	case DIAL_SWITCH_2:
		printf("Dial set to switch 2 \n");
		set_torque_limit_wrapper(0.4);
		break;
	case DIAL_SWITCH_3:
		printf("Dial set to switch 3 \n");
		set_torque_limit_wrapper(0.6);
		break;
	case DIAL_SWITCH_4:
		printf("Dial set to switch 4 \n");
		set_torque_limit_wrapper(0.8);
		break;
	case DIAL_SWITCH_5:
		printf("Dial set to switch 5 \n");
		set_torque_limit_wrapper(1.0);
		break;
	}
}
