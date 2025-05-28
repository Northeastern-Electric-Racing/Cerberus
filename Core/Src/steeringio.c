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

void buttons_update(can_msg_t msg)
{
	uint8_t button_id = msg.data[0];
		
	sm_button_cb(button_id);
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
