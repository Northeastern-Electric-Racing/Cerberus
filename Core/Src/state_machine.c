#include "state_machine.h"
#include "can_handler.h"
#include "fault.h"
#include "nero.h"
#include "pdu.h"
#include "queues.h"
#include "monitor.h"
#include "serial_monitor.h"
#include "nero.h"
#include "queues.h"
#include "processing.h"
#include "dti.h"
#include <stdbool.h>
#include <stdio.h>

#define STATE_TRANS_QUEUE_SIZE 4
#define STATE_TRANSITION_FLAG  1U

/* Internal State of Vehicle */
static state_t cerberus_state;

extern IWDG_HandleTypeDef hiwdg;

osTimerId fault_timer;

typedef struct {
	enum { FUNCTIONAL, NERO } id;
	union {
		func_state_t functional;
		nero_state_t nero;
	} state;
} state_req_t;

/* State Transition Map */
// static const bool valid_trans_to_from[MAX_FUNC_STATES][MAX_FUNC_STATES] = {
// 	/*READY DRIVING FAULTED*/
// 	{ true, true, true }, /* READY */
// 	{ true, true, true }, /* PERFORMANCE */
// 	{ true, false, true } /* FAULTED */
// };

osThreadId_t sm_director_handle;
const osThreadAttr_t sm_director_attributes = {
	.name = "State Machine Director",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime7,
};

static osMessageQueueId_t state_trans_queue;

func_state_t get_func_state()
{
	return cerberus_state.functional;
}

bool get_active()
{
	return cerberus_state.functional == F_EFFICIENCY ||
	       cerberus_state.functional == F_PERFORMANCE ||
	       cerberus_state.functional == F_PIT;
}

nero_state_t get_nero_state()
{
	return cerberus_state.nero;
}

static int transition_functional_state(func_state_t new_state, pdu_t *pdu,
				       dti_t *mc)
{
	// if (!valid_trans_to_from[cerberus_state.functional][new_state]) {
	// 	printf("Invalid State transition");
	// 	return -1;
	// }

	/* Catching state transitions */
	switch (new_state) {
	case READY:
		/* Make sure wheels are not spinning before changing modes */
		if (dti_get_mph(mc) > 1)
			return 1;

		/* Turn off high power peripherals */
		// write_fan_battbox(pdu, false);
		write_pump(pdu, false);
		write_fault(pdu, true);
		serial_print("READY\r\n");
		break;
	case F_PIT:
	case F_PERFORMANCE:
	case F_EFFICIENCY:
		if (cerberus_state.functional != REVERSE) {
			/* Check that motor is not spinning before changing modes */
			if (dti_get_mph(mc) > 1)
				return 2;

			/* Only turn on motor if brakes engaged and tsms is on */
			if (!get_brake_state() || !get_tsms()) {
				return 3;
			}
		}
		sound_rtds(pdu);

		/* Turn on high power peripherals */
		// write_fan_battbox(pdu, true);
		write_pump(pdu, true);
		write_fault(pdu, true);
		serial_print("ACTIVE STATE\r\n");
		break;
	case REVERSE:
		/* Can only enter reverse mode if already in pit mode */
		if (cerberus_state.functional != F_PIT)
			return 4;
		break;
	case FAULTED:
		/* Turn off high power peripherals */
		// write_fan_battbox(pdu, true);
		write_pump(pdu, false);
		write_fault(pdu, false);
		// DO NOT CHANGE WITHOUT CHECKING the bms fault timer, as if BMS timer is later could result in a fault/unfault/fault flicker.
		osTimerStart(fault_timer, 5000);
		HAL_IWDG_Refresh(&hiwdg);
		cerberus_state.nero =
			(nero_state_t){ .nero_index = OFF, .home_mode = true };
		serial_print("FAULTED\r\n");
		break;
	default:
		// Do Nothing
		break;
	}

	cerberus_state.functional = new_state;
	return 0;
}

static int transition_nero_state(nero_state_t new_state, pdu_t *pdu, dti_t *mc)
{
	nero_state_t current_nero_state = get_nero_state();

	// If we are not in home mode, we should not change the nero index
	if (!new_state.home_mode)
		new_state.nero_index = current_nero_state.nero_index;

	// Checks for when we are in home mode and the nero index is out of bounds
	if (new_state.nero_index < 0)
		new_state.nero_index = 0;
	if (new_state.nero_index >= MAX_NERO_STATES)
		new_state.nero_index = MAX_NERO_STATES - 1;

	// Wasn't in home mode and still are not in home mode (Infer a Select Request)
	if (!current_nero_state.home_mode && !new_state.home_mode) {
		// Only Check if we are in pit mode to toggle direction
		if (current_nero_state.nero_index == PIT) {
			if (get_func_state() == REVERSE) {
				if (transition_functional_state(F_PIT, pdu, mc))
					return 1;
			} else if (get_func_state() == F_PIT) {
				if (transition_functional_state(REVERSE, pdu,
								mc))
					return 1;
			}
		}
	}

	// Selecting a mode on NERO
	if (current_nero_state.home_mode && !new_state.home_mode) {
		if (new_state.nero_index < DEBUG &&
		    new_state.nero_index > OFF) {
			if (transition_functional_state(new_state.nero_index,
							pdu, mc))
				return 1;
		}
	}

	// Entering home mode
	if (!current_nero_state.home_mode && new_state.home_mode) {
		if (transition_functional_state(READY, pdu, mc))
			return 1;
	}

	cerberus_state.nero = new_state;
	/* Notify NERO */
	osThreadFlagsSet(nero_monitor_handle, NERO_UPDATE_FLAG);

	return 0;
}

static int queue_state_transition(state_req_t new_state)
{
	if (!state_trans_queue) {
		return 1;
	}

	osStatus_t status =
		osMessageQueuePut(state_trans_queue, &new_state, 0U, 0U);
	osThreadFlagsSet(sm_director_handle, STATE_TRANSITION_FLAG);
	return status;
}

/* HANDLE USER INPUT */
int increment_nero_index()
{
	/* Wrap around if end of menu reached */
	if (get_nero_state().nero_index + 1 == MAX_NERO_STATES) {
		return queue_state_transition((state_req_t){
			.id = NERO,
			.state.nero = (nero_state_t){
				.home_mode = get_nero_state().home_mode,
				.nero_index = OFF } });
	}
	return queue_state_transition((state_req_t){
		.id = NERO,
		.state.nero = (nero_state_t){
			.home_mode = get_nero_state().home_mode,
			.nero_index = get_nero_state().nero_index + 1 } });
}

int decrement_nero_index()
{
	return queue_state_transition((state_req_t){
		.id = NERO,
		.state.nero = (nero_state_t){
			.home_mode = get_nero_state().home_mode,
			.nero_index = get_nero_state().nero_index - 1 } });
}

int select_nero_index()
{
	return queue_state_transition((state_req_t){
		.id = NERO,
		.state.nero = (nero_state_t){
			.home_mode = false,
			.nero_index = get_nero_state().nero_index } });
}

int set_home_mode()
{
	return queue_state_transition((state_req_t){
		.id = NERO,
		.state.nero = { .nero_index = get_nero_state().nero_index,
				.home_mode = true } });
}

/* HANDLE FAULTS */
int fault()
{
	return queue_state_transition(
		(state_req_t){ .id = FUNCTIONAL, .state.functional = FAULTED });
}

void vStateMachineDirector(void *pv_params)
{
	cerberus_state.functional = READY;
	cerberus_state.nero.nero_index = 0;
	cerberus_state.nero.home_mode = true;

	state_trans_queue = osMessageQueueNew(STATE_TRANS_QUEUE_SIZE,
					      sizeof(state_req_t), NULL);

	state_req_t new_state_req;

	pdu_t *pdu = (pdu_t *)((uint32_t *)pv_params)[0];
	dti_t *mc = (dti_t *)((uint32_t *)pv_params)[1];

	for (;;) {
		osThreadFlagsWait(STATE_TRANSITION_FLAG, osFlagsWaitAny,
				  osWaitForever);
		while (osMessageQueueGet(state_trans_queue, &new_state_req,
					 NULL, osWaitForever) == osOK) {
			if (new_state_req.id == NERO)
				transition_nero_state(new_state_req.state.nero,
						      pdu, mc);
			else if (new_state_req.id == FUNCTIONAL)
				transition_functional_state(
					new_state_req.state.functional, pdu,
					mc);
		}
	}
}
