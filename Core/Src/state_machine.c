#include "state_machine.h"
#include "fault.h"
#include "can_handler.h"
#include "serial_monitor.h"
#include "pdu.h"
#include "queues.h"
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#define STATE_TRANS_QUEUE_SIZE 4

typedef struct {
	func_state_t functional;
	drive_state_t drive;
} state_t;

/* Internal State of Vehicle */
static state_t cerberus_state;

static pdu_t *pdu;

extern IWDG_HandleTypeDef hiwdg;

osTimerId fault_timer;

/* State Transition Map */
static const bool valid_trans_to_from[MAX_FUNC_STATES][MAX_FUNC_STATES] = {
	/*BOOT  READY DRIVING FAULTED*/
	{ true, true, false, true },  /* BOOT */
	{ false, true, true, true },  /* READY */
	{ false, true, true, true },  /* DRIVING */
	{ false, true, false, true } /* FAULTED */
};

osThreadId_t sm_director_handle;
const osThreadAttr_t sm_director_attributes =
{
	.name		= "State Machine Director",
	.stack_size = 128 * 8,
	.priority	= (osPriority_t)osPriorityRealtime2,
};

static osMessageQueueId_t state_trans_queue;
osMessageQueueId_t break_state_queue;

void fault_callback()
{
	state_req_t req = {.id = FUNCTIONAL, .state.functional = READY};
	queue_state_transition(req);
}

int queue_state_transition(state_req_t new_state)
{
	if (!state_trans_queue) {
		return 1;
	}

	printf("QUEUING STATAE TRANSITION\r\n");

	if (new_state.id == DRIVE)
	{
		if(get_func_state() != ACTIVE)
			return 0;

		/* Transitioning between drive states is always allowed */
		//TODO: Make sure motor is not spinning before switching

		/* If we are turning ON the motor, blare RTDS */
		if (cerberus_state.drive == NOT_DRIVING) {
			if (!get_brake_state()) {
				return 0;
			}
			serial_print("CALLING RTDS");
			sound_rtds(pdu);
		}

		cerberus_state.drive = new_state.state.drive;
		return 0;
	}

	if (new_state.id == FUNCTIONAL)
	{
		if (!valid_trans_to_from[cerberus_state.functional][new_state.state.functional]) {
			printf("Invalid State transition");
			return -1;
		}

		cerberus_state.functional = new_state.state.functional;
		/* Catching state transitions */
		switch (new_state.state.functional)
		{
			case BOOT:
				/* Do Nothing */
				break;
			case READY:
				/* Turn off high power peripherals */
				serial_print("going to ready");
				write_fan_battbox(pdu, false);
				write_pump(pdu, false);
				write_fault(pdu, true);
				printf("READY\r\n");
				break;
			case ACTIVE:
				/* Turn on high power peripherals */
				write_fan_battbox(pdu, true);
				write_pump(pdu, true);
				write_fault(pdu, true);
				sound_rtds(pdu); // TEMPORARY
				printf("ACTIVE STATE\r\n");
				break;
			case FAULTED:
				/* Turn off high power peripherals */
				write_fan_battbox(pdu, true);
				write_pump(pdu, false);
				write_fault(pdu, false);
				osTimerStart(fault_timer, 5000);
				HAL_IWDG_Refresh(&hiwdg);
				printf("FAULTED\r\n");
				break;
			default:
				// Do Nothing
				break;
		}
		return 0;
	}

		return 0;
}

func_state_t get_func_state()
{
	return cerberus_state.functional;
}

drive_state_t get_drive_state()
{
	return cerberus_state.drive;
}

void vStateMachineDirector(void* pv_params)
{
	cerberus_state.functional = BOOT;
	cerberus_state.drive	  = NOT_DRIVING;

	state_trans_queue = osMessageQueueNew(STATE_TRANS_QUEUE_SIZE, sizeof(state_req_t), NULL);
	break_state_queue = osMessageQueueNew(1, sizeof(bool), NULL);

	pdu_t *pdu_1 = (pdu_t *)pv_params;

	pdu = pdu_1;
	state_req_t new_state;

	serial_print("State Machine Init!\r\n");

	// debug bootup message
	can_msg_t bootup_msg = { .id = 0x701, .len = 8, .data = { 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} };
	queue_can_msg(bootup_msg);

	state_req_t request;
  	request.id = FUNCTIONAL;
  	request.state.functional = READY;
  	queue_state_transition(request);

	fault_timer = osTimerNew(&fault_callback, osTimerOnce, NULL, NULL);

	for (;;)
	{
		osMessageQueueGet(state_trans_queue, &new_state, NULL, osWaitForever);

		if (new_state.id == DRIVE)
		{
			if(get_func_state() != ACTIVE)
				continue;

			/* Transitioning between drive states is always allowed */
			//TODO: Make sure motor is not spinning before switching

			/* If we are turning ON the motor, blare RTDS */
			if (cerberus_state.drive == NOT_DRIVING) {

				/* make sure foot is on break */
				bool break_state = false;
				//osMessageQueueGet(break_state_queue, &break_state, NULL, 0);
				if (!break_state) {
					continue;
				}

				serial_print("CALLING RTDS");
				sound_rtds(pdu);
			}

			cerberus_state.drive = new_state.state.drive;
			continue;
		}

		if (new_state.id == FUNCTIONAL)
		{
			if (!valid_trans_to_from[cerberus_state.functional][new_state.state.functional]) {
				printf("Invalid State transition");
				continue;
			}

			cerberus_state.functional = new_state.state.functional;
			/* Catching state transitions */
			switch (new_state.state.functional)
			{
				case BOOT:
					/* Do Nothing */
					break;
				case READY:
					/* Turn off high power peripherals */
					serial_print("going to ready");
					write_fan_battbox(pdu, false);
					write_pump(pdu, true);
					write_fault(pdu, true);
					break;
				case ACTIVE:
					/* Turn on high power peripherals */
					write_fan_battbox(pdu, true);
					write_pump(pdu, true);
					write_fault(pdu, true);
					break;
				case FAULTED:
					/* Turn off high power peripherals */
					write_fan_battbox(pdu, false);
					write_pump(pdu, false);
					write_fault(pdu, false);
					HAL_IWDG_Refresh(&hiwdg);
					while(1) {printf("Delaying...\r\n");}
					break;
				default:
					// Do Nothing
					break;
			}
			continue;
		}
	}
}

