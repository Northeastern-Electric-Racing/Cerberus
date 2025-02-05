#include "state_machine.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cerb_utils.h"
#include "monitor.h"
#include "pedals.h"

#define STATE_TRANS_QUEUE_SIZE 4

#define SEND_NERO_TIMEOUT 500 /*in millis*/

// #define DISABLE_REVERSE

/* Internal State of Vehicle */
static state_t cerberus_state;

typedef struct {
	enum { FUNCTIONAL, NERO } id;
	union {
		func_state_t functional;
		nero_state_t nero;
	} state;
} state_req_t;

osThreadId_t sm_director_handle;
const osThreadAttr_t sm_director_attributes = {
	.name = "State Machine Director",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime7,
};

static osMessageQueueId_t state_trans_queue;

static void send_nero_msg(dti_t *mc)
{
	struct __attribute__((__packed__)) {
		uint8_t home_mode;
		uint8_t nero_index;
		uint8_t mph;
		uint8_t tsms;
		uint8_t torque_lim_percentage;
	} nero_data;

	nero_data.home_mode = (uint8_t)get_nero_state().home_mode;
	nero_data.mph = dti_get_mph(mc);
	nero_data.tsms = (uint8_t)get_tsms();
	/* Percentage from 0 - 1, multiplied by 100 */
	nero_data.torque_lim_percentage =
		(uint8_t)(get_torque_limit_percentage() * 100);

	can_msg_t msg = { .id = 0x501, .len = sizeof(nero_data) };

	memcpy(&msg.data, &nero_data, sizeof(nero_data));

	/* Send CAN message */
	queue_can_msg(msg);
}

func_state_t get_func_state()
{
	return cerberus_state.functional;
}

bool get_active()
{
	return cerberus_state.functional == F_EFFICIENCY ||
	       cerberus_state.functional == F_PERFORMANCE ||
	       cerberus_state.functional == F_PIT ||
	       cerberus_state.functional == F_REVERSE;
}

nero_state_t get_nero_state()
{
	return cerberus_state.nero;
}

static int transition_functional_state(func_state_t new_state, pdu_t *pdu,
				       dti_t *mc, mpu_t *mpu)
{
	/* Make sure wheels are not spinning before changing modes */
	if (dti_get_mph(mc) > 1)
		return 1;
	bool brake_state;

	/* Catching state transitions */
	switch (new_state) {
	case READY:
		/* Turn off high power peripherals */
		// write_fan_battbox(pdu, false);
		write_pump(pdu, false);
		write_fault(mpu, false);
		printf("READY\r\n");
		break;
	case F_PIT:
	case F_PERFORMANCE:
	case F_EFFICIENCY:
		if (read_brake_state(pdu, &brake_state)) {
			return 3;
		}
#ifdef TSMS_OVERRIDE
		if (!brake_state) {
			return 3;
		}
#else
		/* Only turn on motor if brakes engaged and tsms is on */
		if (!brake_state || !get_tsms()) {
			return 3;
		}
#endif
		osThreadFlagsSet(rtds_thread, SOUND_RTDS_FLAG);

		/* Turn on high power peripherals */
		// write_fan_battbox(pdu, true);
		write_pump(pdu, true);
		write_fault(mpu, false);
		printf("ACTIVE STATE\r\n");
		break;

	case F_REVERSE:
#ifndef DISABLE_REVERSE
		/* Can only enter reverse mode if already in pit mode */
		if (cerberus_state.functional != F_PIT)
			return 4;
#endif
		break;
	case FAULTED:
		/* Turn off high power peripherals */
		// write_fan_battbox(pdu, true);
		write_pump(pdu, false);
		cerberus_state.nero =
			(nero_state_t){ .nero_index = OFF, .home_mode = false };
		write_fault(mpu, true);
		printf("FAULTED\r\n");
		break;
	default:
		// Do Nothing
		break;
	}

	cerberus_state.functional = new_state;

	return 0;
}

static int transition_nero_state(nero_state_t new_state, pdu_t *pdu, dti_t *mc,
				 mpu_t *mpu)
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

	// Selecting a mode on NERO
	if (current_nero_state.home_mode && !new_state.home_mode) {
		if (new_state.nero_index < DEBUG &&
		    new_state.nero_index > OFF) {
			if (transition_functional_state(new_state.nero_index,
							pdu, mc, mpu))
				return 1;
		}

		/* TSMS and MPH = 0 to enter games */
		if (new_state.nero_index == GAMES) {
			if (!get_tsms() && dti_get_mph(mc) <= 1) {
				return 1;
			}

			if (transition_functional_state(new_state.nero_index,
							pdu, mc, mpu))
				return 1;
		}
	}

	// Entering home mode
	if (!current_nero_state.home_mode && new_state.home_mode) {
		if (transition_functional_state(READY, pdu, mc, mpu))
			return 1;
	}

	cerberus_state.nero = new_state;

	return 0;
}

static int check_state_change(state_req_t new_state)
{
	// check if nero state has changed
	if (new_state.id == NERO) {
		nero_state_t new_nero_state = new_state.state.nero;
		nero_state_t current_nero_state = get_nero_state();
		if (new_nero_state.home_mode == current_nero_state.home_mode &&
		    new_nero_state.nero_index ==
			    current_nero_state.nero_index) {
			return 0;
		}
	}

	// check if functional state has changed
	if (new_state.id == FUNCTIONAL) {
		func_state_t new_func_state = new_state.state.functional;
		func_state_t current_func_state = get_func_state();
		if (new_func_state == current_func_state) {
			return 0;
		}
	}

	return 1;
}

static int queue_state_transition(state_req_t new_state)
{
	if (!state_trans_queue) {
		return 1;
	}

	return osMessageQueuePut(state_trans_queue, &new_state, 0U, 0U);
}

/* HANDLE USER INPUT */
int increment_nero_index()
{
	/* Wrap around if end of menu reached */
	if (get_nero_state().nero_index + 1 >= MAX_NERO_STATES) {
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

int set_ready_mode()
{
	return queue_state_transition(
		(state_req_t){ .id = FUNCTIONAL, .state.functional = READY });
}

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

	sm_director_args_t *args = (sm_director_args_t *)pv_params;
	pdu_t *pdu = args->pdu;
	assert(pdu);
	dti_t *mc = args->mc;
	assert(mc);
	mpu_t *mpu = args->mpu;
	assert(mpu);

	free(args);

	/* Write to GPIO expander to set initial state */
	write_pump(pdu, false);
	write_fault(mpu, false);

	for (;;) {
		if (osMessageQueueGet(state_trans_queue, &new_state_req, NULL,
				      pdMS_TO_TICKS(SEND_NERO_TIMEOUT)) ==
			    osOK &&
		    check_state_change(new_state_req)) {
			if (new_state_req.id == NERO)
				transition_nero_state(new_state_req.state.nero,
						      pdu, mc, mpu);
			else if (new_state_req.id == FUNCTIONAL)
				transition_functional_state(
					new_state_req.state.functional, pdu, mc,
					mpu);
		}

		// send nero data periodically
		send_nero_msg(mc);
	}
}
