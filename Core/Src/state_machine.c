#include "state_machine.h"
#include "can_handler.h"
#include "fault.h"
#include "nero.h"
#include "pdu.h"
#include "queues.h"
#include "monitor.h"
#include "serial_monitor.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define STATE_TRANS_QUEUE_SIZE 4

/* Internal State of Vehicle */
static state_t cerberus_state;

static pdu_t *pdu;

extern IWDG_HandleTypeDef hiwdg;

osTimerId fault_timer;

/* State Transition Map */
static const bool valid_trans_to_from[MAX_FUNC_STATES][MAX_FUNC_STATES] = {
	/*BOOT  READY DRIVING FAULTED*/
	{ true, true, false, true }, /* BOOT */
	{ false, true, true, true }, /* READY */
	{ false, true, true, true }, /* DRIVING */
	{ false, true, false, true } /* FAULTED */
};

osThreadId_t sm_director_handle;
const osThreadAttr_t sm_director_attributes = {
	.name = "State Machine Director",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime2,
};

static osMessageQueueId_t state_trans_queue;

func_state_t get_func_state()
{
	return cerberus_state.functional;
}

drive_state_t get_drive_state()
{
	return cerberus_state.drive;
}

nero_state_t get_nero_state()
{
	return cerberus_state.nero;
}

static drive_state_t map_nero_index_to_drive_state(nero_menu_t nero_index)
{
	switch (nero_index) {
	case OFF:
		return NOT_DRIVING;
	case PIT:
		return SPEED_LIMITED;
	case PERFORMANCE:
		return AUTOCROSS;
	case EFFICIENCY:
		return ENDURANCE;
	default:
		return OFF;
	}
}

static int transition_drive_state(drive_state_t new_state) {
		if (get_func_state() != ACTIVE)
			return 0;

		/* Transitioning between drive states is always allowed */
		// TODO: Make sure motor is not spinning before switching

		/* If we are turning ON the motor, blare RTDS */
		if (cerberus_state.drive == NOT_DRIVING) {
			if (!get_brake_state()) {
				return 0;
			}
			serial_print("CALLING RTDS");
			sound_rtds(pdu);
		}

		cerberus_state.drive = new_state;
		return 0;
}

static int transition_functional_state(func_state_t new_state) {
	if (!valid_trans_to_from[cerberus_state.functional][new_state]) {
		printf("Invalid State transition");
		return -1;
	}

	/* Catching state transitions */
	switch (new_state) {
		case BOOT:
			/* Do Nothing */
			break;
		case READY:
			/* Turn off high power peripherals */
			// write_fan_battbox(pdu, false);
			write_pump(pdu, false);
			write_fault(pdu, true);
			transition_drive_state(OFF);
			printf("READY\r\n");
			break;
		case ACTIVE:
			/* Turn on high power peripherals */
			// write_fan_battbox(pdu, true);
			write_pump(pdu, true);
			write_fault(pdu, true);
			sound_rtds(pdu); // TEMPORARY
			printf("ACTIVE STATE\r\n");
			break;
		case FAULTED:
			/* Turn off high power peripherals */
			// write_fan_battbox(pdu, true);
			write_pump(pdu, false);
			write_fault(pdu, false);
			// DO NOT CHANGE WITHOUT CHECKING the bms fault timer, as if BMS timer is later could result in a fault/unfault/fault flicker.
			osTimerStart(fault_timer, 5000);
			HAL_IWDG_Refresh(&hiwdg);
			transition_drive_state(OFF);
			cerberus_state.nero = (nero_state_t){ .nero_index = OFF, .home_mode = true };
			printf("FAULTED\r\n");
			break;
		default:
			// Do Nothing
			break;
	}

	cerberus_state.functional = new_state;
	return 0;
}

static int transition_nero_state(nero_state_t new_state) {
	nero_state_t current_nero_state = get_nero_state();

	// If we are not in home mode, we should not change the nero index
	if (!new_state.home_mode) new_state.nero_index = current_nero_state.nero_index; 

	// Checks for when we are in home mode and the nero index is out of bounds
	if (new_state.nero_index < 0) new_state.nero_index = 0;
	if (new_state.nero_index >= MAX_NERO_STATES) new_state.nero_index = MAX_NERO_STATES - 1;

	// Wasn't in home mode and still are not in home mode (Infer a Select Request)
	if (!current_nero_state.home_mode && !new_state.home_mode) {
		// Only Check if we are in pit mode to toggle direction
		if (current_nero_state.nero_index == PIT) {
			if (get_drive_state() == REVERSE) {
				if (transition_drive_state(SPEED_LIMITED)) return 1;
			} else {
				if (transition_drive_state(REVERSE)) return 1;
			}
		}
	}

	uint8_t max_drive_states = MAX_DRIVE_STATES - 1; // Account for reverse and pit being the same screen

	// Selecting a mode on NERO
	if (current_nero_state.home_mode && !new_state.home_mode) {
		if (new_state.nero_index > 0 &&
	    	 	new_state.nero_index < max_drive_states && 
		 		get_tsms() && get_brake_state()) {
			if (transition_functional_state(ACTIVE)) return 1;
			if (transition_drive_state(map_nero_index_to_drive_state(new_state.nero_index))) return 1;
		} else if (new_state.nero_index == OFF) {
			if (transition_functional_state(READY)) return 1;
		}
	}

	// Entering home mode
	if (!current_nero_state.home_mode && new_state.home_mode) {
		if (transition_functional_state(READY)) return 1;
	}

	cerberus_state.nero = new_state;
	return 0;
}

static int queue_state_transition(state_t new_state)
{
	if (!state_trans_queue) {
		return 1;
	}

	return osMessageQueuePut(state_trans_queue, &new_state, 0U, 0U);
}

/* HANDLE USER INPUT */
int increment_nero_index()
{
	return queue_state_transition((state_t){ 
			.functional = get_func_state(), 
			.drive = get_drive_state(), 
			.nero = { 
				.nero_index = get_nero_state().nero_index + 1, 
				.home_mode = get_nero_state().home_mode 
				} 
			});
}

int decrement_nero_index()
{
	return queue_state_transition((state_t){ 
			.functional = get_func_state(), 
			.drive = get_drive_state(), 
			.nero = { 
				.nero_index = get_nero_state().nero_index - 1, 
				.home_mode = get_nero_state().home_mode 
				} 
			});
}

int select_nero_index()
{
	return queue_state_transition((state_t){ 
			.functional = get_func_state(), 
			.drive = get_drive_state(), 
			.nero = { 
				.nero_index = get_nero_state().nero_index, 
				.home_mode = false
				} 
			});
}

int set_home_mode()
{
	return queue_state_transition((state_t){ 
			.functional = get_func_state(), 
			.drive = get_drive_state(), 
			.nero = { 
				.nero_index = get_nero_state().nero_index, 
				.home_mode = true
				} 
			});
}

/* HANDLE FAULTS */
int fault() {
	return queue_state_transition((state_t){ 
			.functional = FAULTED, 
			.drive = get_drive_state(), 
			.nero = get_nero_state() 
		});
}

void vStateMachineDirector(void *pv_params)
{
	cerberus_state.functional = BOOT;
	cerberus_state.drive = NOT_DRIVING;
	cerberus_state.nero.nero_index = 0;
	cerberus_state.nero.home_mode = true;

	state_trans_queue = osMessageQueueNew(STATE_TRANS_QUEUE_SIZE,
					      sizeof(state_t), NULL);

	pdu_t *pdu_1 = (pdu_t *)pv_params;

	pdu = pdu_1;
	state_t new_state;

	serial_print("State Machine Init!\r\n");

	for (;;) { 
		osMessageQueueGet(state_trans_queue, &new_state, NULL,
				  osWaitForever);

		if (transition_drive_state(new_state.drive)) continue;

		if (transition_nero_state(new_state.nero)) continue;

		if (transition_functional_state(new_state.functional)) continue;
	}
}
