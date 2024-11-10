#include "steeringio.h"
#include "can.h"
#include "cerb_utils.h"
#include "cerberus_conf.h"
#include "cmsis_os.h"
#include "nero.h"
#include "pedals.h"
#include "state_machine.h"
#include "stdio.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CAN_QUEUE_SIZE 5 /* messages */

static void paddle_left_cb()
{
	if (get_func_state() == F_EFFICIENCY) {
		increase_torque_limit();
	}
}

static void paddle_right_cb()
{
	if (get_func_state() == F_EFFICIENCY) {
		decrease_torque_limit();
	}
}

void steeringio_update(can_msg_t msg)
{
	uint8_t button_id = msg.data[0];

	switch (button_id) {
	case STEERING_PADDLE_LEFT:
		paddle_left_cb();
		break;
	case STEERING_PADDLE_RIGHT:
		paddle_right_cb();
		break;
	case NERO_BUTTON_UP:
		printf("Up button pressed \r\n");
		decrement_nero_index();
		break;
	case NERO_BUTTON_DOWN:
		printf("Down button pressed \r\n");
		increment_nero_index();
		break;
	case NERO_BUTTON_LEFT:
		// doesnt effect cerb for now
		break;
	case NERO_BUTTON_RIGHT:
		// doesnt effect cerb for now
		break;
	case NERO_BUTTON_SELECT:
		printf("Select button pressed \r\n");
		select_nero_index();
		break;
	case NERO_HOME:
		printf("Home button pressed \r\n");
		set_home_mode();
		break;
	default:
		break;
	}
}
