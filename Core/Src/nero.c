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
#include "processing.h"

static int8_t mph = 0;

void send_nero_msg()
{
	can_msg_t msg = { .id = 0x501,
			  .len = 4,
			  .data = { get_nero_state().home_mode,
				    get_nero_state().nero_index, mph,
				    get_tsms() } };

	/* Send CAN message */
	queue_can_msg(msg);
}

void set_mph(int8_t new_mph)
{
	mph = new_mph;
	send_nero_msg();
}