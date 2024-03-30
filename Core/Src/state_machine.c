#include "state_machine.h"
#include "fault.h"
#include "serial_monitor.h"
#include <stdbool.h>
#include <stdio.h>

#define STATE_TRANS_QUEUE_SIZE 16

typedef struct 
{
	func_state_t functional;
	drive_state_t drive;
} state_t;

/* Internal State of Vehicle */
static state_t cerberus_state;

static int nero_index = 0;

static bool home_mode = false;

/* State Transition Map */
static const bool valid_trans_to_from[MAX_FUNC_STATES][MAX_FUNC_STATES] = {
	/*BOOT  READY DRIVING FAULTED*/
	{ true, true, false, true },  /* BOOT */
	{ false, true, true, true },  /* READY */
	{ false, true, true, true },  /* DRIVING */
	{ false, true, false, false } /* FAULTED */
};

osThreadId_t sm_director_handle;
const osThreadAttr_t sm_director_attributes = 
{
	.name		= "State Machine Director",
	.stack_size = 128 * 4,
	.priority	= (osPriority_t)osPriorityAboveNormal3,
};

static osMessageQueueId_t func_state_trans_queue;
static osMessageQueueId_t drive_state_trans_queue;

int queue_func_state(func_state_t new_state)
{
	if (!state_trans_queue)
		return 1;

	state_t queue_state = { .functional = new_state };

	serial_print("Queued Functional State!\r\n");

	return osMessageQueuePut(func_state_trans_queue, &queue_state, 0U, 0U);
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

	return osMessageQueuePut(drive_state_trans_queue, &queue_state, 0U, 0U);
}

void increment_nero_index() {
	if (!home_mode) {
		// Do Nothing because we are not in home mode and therefore not tracking the nero index
		return;
	}

	if (nero_index += 1 < MAX_DRIVE_STATES) {
		nero_index += 1;
	} else {
		// Do Nothing because theres no additional states or we dont care about the additional states;
	}
}

void decrement_nero_index() {
	if (!home_mode) {
		// Do Nothing because we are not in home mode and therefore not tracking the nero index
		return;
	}

	if (nero_index -= 1 >= 0) {
		nero_index -= 1;
	} else {
		// Do Nothing because theres no negative states
	}
}

void select_nero_index() {
	if (!home_mode) {
		// Do Nothing because we are not in home mode and therefore not tracking the nero index
		return;
	}

	if (nero_index >= 0 && nero_index < MAX_DRIVE_STATES) {
		home_mode = false;
		queue_drive_state(nero_index);
	} else {
		// Do Nothing because the index is out of bounds
	}
}

void set_home_mode() {
	home_mode = true;
}

drive_state_t get_drive_state()
{
	return cerberus_state.drive;
}

void vStateMachineDirector(void* pv_params)
{
	cerberus_state.functional = BOOT;
	cerberus_state.drive	  = NOT_DRIVING;

	func_state_trans_queue = osMessageQueueNew(STATE_TRANS_QUEUE_SIZE, sizeof(state_t), NULL);
	drive_state_trans_queue = osMessageQueueNew(STATE_TRANS_QUEUE_SIZE, sizeof(state_t), NULL);

	state_t new_state;

	serial_print("State Machine Init!\r\n");

	for (;;) 
	{
		if (osOK != osMessageQueueGet(func_state_trans_queue, &new_state, NULL, 50)) 
		{
			fault_data_t state_trans_fault = {STATE_RECEIVED_FAULT, DEFCON4, "Failed to transition state"};
			if(queue_fault(&state_trans_fault) == -1)
			{
				serial_print("Fill this in later");
			}
			continue;
		}

		if (osOk != osMessageQueueGet(drive_state_trans_queue, &new_state, NULL, 50))
		{
			fault_data_t state_trans_fault = {STATE_RECEIVED_FAULT, DEFCON4, "Failed to transition state"};
			if(queue_fault(&state_trans_fault) == -1)
			{
				serial_print("Fill this in later");
			}
			continue;
		}
		
		if (!valid_trans_to_from[new_state.functional][cerberus_state.functional]) 
		{
			fault_data_t invalid_trans_fault = {INVALID_TRANSITION_FAULT, DEFCON5, "Failed to transition state"};
			if(queue_fault(&state_trans_fault) == -1)
			{
				serial_print("Fill this in later")
			}
			continue;
		}
		
		if(cerberus_state.functional == BOOT)
		{
			// TODO: Add some more checks here to make sure we are good to go into ready
			// Dont actually know exactly what we should be checking for
			queue_func_state(READY);
		}

		cerberus_state.functional = new_state.functional;
		cerberus_state.drive	  = new_state.functional == DRIVING ? new_state.drive : NOT_DRIVING;
	}
}
