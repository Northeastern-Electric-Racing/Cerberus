#include "nero.h"
#include "c_utils.h"
#include "can_handler.h"
#include "cerberus_conf.h"
#include "monitor.h"
#include "pedals.h"
#include "queues.h"
#include "state_machine.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"

// #define TORQUE_DEBUG

static int8_t mph = 0;

void send_nero_msg()
{
	struct __attribute__((__packed__)) {
		uint8_t home_mode;
		uint8_t nero_index;
		uint8_t mph;
		uint8_t tsms;
		uint8_t torque_lim_percentage;
	} nero_data;

	/* Since the screen on NERO relies on the NERO index, and reverse and pit have the same index,
	 * reverse gets a special index */
	if (get_func_state() == REVERSE) {
		nero_data.nero_index = 255;
	} else {
		nero_data.nero_index = (uint8_t)get_nero_state().nero_index;
	}

	nero_data.home_mode = (uint8_t)get_nero_state().home_mode;
	nero_data.mph = mph;
	nero_data.tsms = (uint8_t)get_tsms();
	/* Percentage from 0 - 1, multiplied by 100 */
	nero_data.torque_lim_percentage =
		(uint8_t)(get_torque_limit_percentage() * 100);

	can_msg_t msg = { .id = 0x501, .len = sizeof(nero_data) };

	memcpy(&msg.data, &nero_data, sizeof(nero_data));

	/* Send CAN message */
	queue_can_msg(msg);
}

void set_mph(int8_t new_mph)
{
	mph = new_mph;
	send_nero_msg();
}