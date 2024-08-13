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
#include "pedals.h"

static int8_t mph = 0;

void send_nero_msg()
{
	uint8_t nero_index;
	/* Since the screen on NERO relies on the NERO index, and reverse and pit have the same index, reverse gets a special index */
	if (get_func_state() == REVERSE) {
		nero_index = 255;
	} else {
		nero_index = get_nero_state().nero_index;
	}
	can_msg_t msg = { .id = 0x501,
			  .len = 4,
			  .data = { get_nero_state().home_mode, nero_index, mph,
				    get_tsms() } };

	/* Send CAN message */
	queue_can_msg(msg);
}

void set_mph(int8_t new_mph)
{
	mph = new_mph;
	send_nero_msg();
}