#include "state_machine.h"
#include "fault.h"
#include "can_handler.h"
#include "serial_monitor.h"
#include "pdu.h"
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
	if (!func_state_trans_queue)
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
	if (!drive_state_trans_queue)
		return 1;

	if (cerberus_state.functional != DRIVING)
		return -1;

	state_t queue_state = { .functional = DRIVING, .drive = new_state };

	serial_print("Queued Drive State!\r\n");

	return osMessageQueuePut(drive_state_trans_queue, &queue_state, 0U, 0U);
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

	pdu_t *pdu = (pdu_t *)pv_params;

	state_t new_state;

	serial_print("State Machine Init!\r\n");

	for (;;) 
	{
		if (osOK != osMessageQueueGet(func_state_trans_queue, &new_state, NULL, osWaitForever)) 
		{
			fault_data_t state_trans_fault = {STATE_RECEIVED_FAULT, DEFCON4, "Failed to transition functional state"};
			if(queue_fault(&state_trans_fault) == -1)
			{
				serial_print("Fill this in later");
			}
			continue;
		}

		if (osOK != osMessageQueueGet(drive_state_trans_queue, &new_state, NULL, osWaitForever))
		{
			fault_data_t state_trans_fault = {STATE_RECEIVED_FAULT, DEFCON4, "Failed to transition drive state"};
			if(queue_fault(&state_trans_fault) == -1)
			{
				serial_print("Fill this in later");
			}
			continue;
		}
		
		if (!valid_trans_to_from[new_state.functional][cerberus_state.functional]) 
		{
			fault_data_t invalid_trans_fault = {INVALID_TRANSITION_FAULT, DEFCON5, "Invalid state transition"};
			if(queue_fault(&invalid_trans_fault) == -1)
			{
				serial_print("Fill this in later");
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

		switch (new_state.functional)
		{
			case BOOT:
				// Do Nothing
				break;
			case READY:
				// Turn off shit
				write_fan_battbox(pdu, false);
				write_fan_radiator(pdu, false);
				write_pump(pdu, false);
				break;
			case DRIVING:
				// Turn on shit
				write_fan_battbox(pdu, true);
				write_fan_radiator(pdu, true);
				write_pump(pdu, true);
				break;
			case FAULTED:
				// Turn off shit
				write_fan_battbox(pdu, false);
				write_fan_radiator(pdu, false);
				write_pump(pdu, false);
				break;
			default:
				// Do Nothing
				break;
		}
	}
}

