#include "state_machine.h"
#include "fault.h"
#include "serial_monitor.h"
#include <stdbool.h>
#include <stdio.h>

#define STATE_TRANS_QUEUE_SIZE 16

typedef struct {
	func_state_t functional;
	drive_state_t drive;
} state_t;

/* Internal State of Vehicle */
static state_t cerberus_state;

/* State Transition Map */
static const bool valid_trans_to_from[MAX_FUNC_STATES][MAX_FUNC_STATES] = {
	/*BOOT  READY   DRIVING FAULTED*/
	{ true, true, false, true },  /* BOOT */
	{ false, true, true, true },  /* READY */
	{ false, true, true, true },  /* DRIVING */
	{ false, true, false, false } /* FAULTED */
};

osThreadId_t sm_director_handle;
const osThreadAttr_t sm_director_attributes = {
	.name		= "State Machine Director",
	.stack_size = 128 * 4,
	.priority	= (osPriority_t)osPriorityAboveNormal3,
};

static osMessageQueueId_t state_trans_queue;

int queue_func_state(func_state_t new_state)
{
	if (!state_trans_queue)
		return 1;

	state_t queue_state = { .functional = new_state };

	serial_print("Queued Functional State!\r\n");

	return osMessageQueuePut(state_trans_queue, &queue_state, 0U, 0U);
}

func_state_t get_func_state()
{
	return cerberus_state.functional;
}

int queue_drive_state(drive_state_t new_state)
{
	if (!state_trans_queue)
		return 1;

	if (cerberus_state.functional != DRIVING)
		return -1;

	state_t queue_state = { .functional = DRIVING, .drive = new_state };

	serial_print("Queued Drive State!\r\n");

	return osMessageQueuePut(state_trans_queue, &queue_state, 0U, 0U);
}

drive_state_t get_drive_state()
{
	return cerberus_state.drive;
}

void vStateMachineDirector(void* pv_params)
{
	cerberus_state.functional = BOOT;
	cerberus_state.drive	  = NOT_DRIVING;

	state_trans_queue = osMessageQueueNew(STATE_TRANS_QUEUE_SIZE, sizeof(state_t), NULL);

	state_t new_state;

	serial_print("State Machine Init!\r\n");

	for (;;) {
		if (osOK != osMessageQueueGet(state_trans_queue, &new_state, NULL, 50)) {
			// TODO queue fault, low criticality
			continue;
		}
		serial_print("BRUH\r\n");
		// serial_print("New State received:\t%d\r\n", new_state.functional);
		// serial_print("Current state:\t%d\r\n", cerberus_state.functional);

		if (!valid_trans_to_from[new_state.functional][cerberus_state.functional]) {
			// TODO queue fault, low criticality
			continue;
		}

		// transition state via LUT ?
		serial_print("Transitioned State!\r\n");
		cerberus_state.functional = new_state.functional;
		cerberus_state.drive	  = new_state.functional == DRIVING ? new_state.drive : NOT_DRIVING;
	}
}
