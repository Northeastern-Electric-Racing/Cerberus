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

static void send_mode_status()
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
}

osThreadId_t nero_monitor_handle;
const osThreadAttr_t nero_monitor_attributes = {
	.name = "NeroMonitor",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityNormal,
};

void vNeroMonitor(void *pv_params)
{
	for (;;) {
		send_mode_status();

		osDelay(NERO_DELAY_TIME);
	}
}