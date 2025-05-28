#include "state_machine.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dti.h"
#include "monitor.h"
#include "pedals.h"
#include "can_handler.h"

#define STATE_TRANS_QUEUE_SIZE 4

#define SEND_NERO_TIMEOUT	200 /*in millis*/
#define TS_RISING_BLOCK_TIMEOUT 3000 /*in millis*/


/* Internal State of Vehicle */

static state_t cerberus_state;
static state_handler_t functional_states[MAX_FUNC_STATES];

const bool valid_transition_from_to[MAX_FUNC_STATES][MAX_FUNC_STATES] = {
	/*   READY,     FAULTED,      ACTIVE STATES,  */
	{ true, true, true, true, true, true }, /* READY */
	{ true, true, false, false, false, false }, /* FAULTED */
	{ true, true, false, false, false, false }, /* ACTIVE STATES */
	{ true, true, false, false, false, false },
	{ true, true, false, false, false, false },
	{ true, true, false, false, false, false } 
};

void sm_button_cb(steeringio_button_t button_id) {
	if (functional_states[cerberus_state.functional].button_cbs[button_id] == NULL) {
		return;
	}
	functional_states[cerberus_state.functional].button_cbs[button_id]();
}

static uint8_t init_ready(mpu_t* mpu) {
	write_fault(mpu, false);
	return 0;
}

static uint8_t init_active(mpu_t* mpu) {
	if (!get_tsms() && !get_brake_state() && !enter_drive_enabled) {
		return 1;
	}
	osThreadFlagsSet(rtds_thread, SOUND_RTDS_FLAG);
	return 0;
}

static uint8_t init_faulted(mpu_t* mpu) {
	cerberus_state.nero =
			(nero_state_t){ .nero_index = OFF, .home_mode = true };
	write_fault(mpu, false);
	return 0;
}

static void ready_left_cb() {
	decrement_nero_index();
}

static void ready_right_cb() {
	increment_nero_index();
}

static void ready_enter_cb() {
	select_nero_index();
}

static void endurance_up_cb() {
	increase_regen_limit();
}

static void endurance_down_cb() {
	decrease_regen_limit();
}

static void endurance_right_cb() {
	increase_regen_limit();
}

static void endurance_left_cb() {
	decrease_regen_limit();
}

static void performance_enter_cb() {
	toggle_launch_control();
}

static void escape_cb() {
	set_home_mode();
}

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
static osTimerId_t ts_rising_timer;
static bool is_ts_rising = false;
static bool enter_drive_enabled = false;

static void send_nero_msg(dti_t *mc)
{
	bitstream_t nero_msg;
	uint8_t bitstream_data[6];
	bitstream_init(&nero_msg, bitstream_data,
		       6); // Create 5-byte bitstream

	bitstream_add(&nero_msg, get_nero_state().home_mode, 4);
	bitstream_add(&nero_msg, get_nero_state().nero_index, 4);
	bitstream_add(&nero_msg, dti_get_mph(mc) * 10, 16);
	bitstream_add(&nero_msg, get_tsms(), 1);
	bitstream_add(&nero_msg, get_torque_limit_percentage() * 100, 7);
	bitstream_add(&nero_msg, cerberus_state.functional != F_REVERSE, 1);
	bitstream_add(&nero_msg, get_regen_limit(), 10);
	bitstream_add(&nero_msg, get_launch_control(), 1);

	can_msg_t msg = { .id = 0x501, .len = sizeof(bitstream_data) };

	memcpy(msg.data, &bitstream_data, sizeof(bitstream_data));

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
	if (new_state == cerberus_state.functional) {
		return 1;
	} 

	if (!valid_transition_from_to[cerberus_state.functional][new_state]) {
		return 1;
	}

	if (!functional_states[cerberus_state.functional].init_func(mpu)) {
		return 1;
	}

	cerberus_state.functional = new_state;

	return 0;
}

static int transition_nero_state(nero_state_t new_state, pdu_t *pdu, dti_t *mc,
				 mpu_t *mpu)
{
	nero_state_t current_nero_state = cerberus_state.nero;

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
		if (new_state.nero_index < GAMES &&
		    new_state.nero_index > OFF) {
			if (transition_functional_state(new_state.nero_index,
							pdu, mc, mpu))
				return 1;
		}

		/* TSMS OFF and MPH = 0 to enter games */
		if (new_state.nero_index == GAMES) {
#ifndef TSMS_OVERRIDE
			if (get_tsms() || dti_get_mph(mc) >= 1) {
				return 1;
			}
#endif
			new_state.home_mode = false;
		}
	}

	// Entering home mode
	if (get_active() && !current_nero_state.home_mode &&
	    new_state.home_mode) {
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
#ifdef IGNORE_FAULT
	return 1;
#endif
	return queue_state_transition(
		(state_req_t){ .id = FUNCTIONAL, .state.functional = READY });
}

int fault()
{
#ifdef IGNORE_FAULT
	return 1;
#endif
	return queue_state_transition(
		(state_req_t){ .id = FUNCTIONAL, .state.functional = FAULTED });
}

void rising_ts_cb(void *args)
{
	enter_drive_enabled = true;
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

	ts_rising_timer = osTimerNew(rising_ts_cb, osTimerOnce, NULL, NULL);

	functional_states[READY].init_func = init_ready;
	functional_states[READY].button_cbs[BUTTON_UP] = ready_right_cb;
	functional_states[READY].button_cbs[BUTTON_DOWN] = ready_left_cb;
	functional_states[READY].button_cbs[BUTTON_RIGHT] = ready_right_cb;
	functional_states[READY].button_cbs[BUTTON_LEFT] = ready_left_cb;
	functional_states[READY].button_cbs[BUTTON_ENTER] = ready_enter_cb;

	functional_states[FAULTED].init_func = init_faulted;

	functional_states[F_PERFORMANCE].init_func = init_active;
	functional_states[F_PERFORMANCE].button_cbs[BUTTON_ENTER] = performance_enter_cb;
	functional_states[F_PERFORMANCE].button_cbs[BUTTON_ESC] = escape_cb;

	functional_states[F_PERFORMANCE].init_func = init_active;
	functional_states[F_EFFICIENCY].button_cbs[BUTTON_UP] = endurance_up_cb;
	functional_states[F_EFFICIENCY].button_cbs[BUTTON_DOWN] = endurance_down_cb;
	functional_states[F_EFFICIENCY].button_cbs[BUTTON_RIGHT] = endurance_right_cb;
	functional_states[F_EFFICIENCY].button_cbs[BUTTON_LEFT] = endurance_left_cb;
	functional_states[F_PERFORMANCE].button_cbs[BUTTON_ESC] = escape_cb;

	functional_states[F_PERFORMANCE].init_func = init_active;
	functional_states[F_PERFORMANCE].button_cbs[BUTTON_ENTER] = performance_enter_cb;
	functional_states[F_PERFORMANCE].button_cbs[BUTTON_ESC] = escape_cb;

	functional_states[F_PIT].init_func = init_active;
	functional_states[F_PIT].button_cbs[BUTTON_ESC] = escape_cb;

	functional_states[F_REVERSE].init_func = init_active;
	functional_states[F_REVERSE].button_cbs[BUTTON_ESC] = escape_cb;
	
	// call initial state
	functional_states[cerberus_state.functional].init_func(mpu);

	for (;;) {
		if (osMessageQueueGet(state_trans_queue, &new_state_req, NULL,
				      pdMS_TO_TICKS(SEND_NERO_TIMEOUT)) ==
		    osOK) {
			if (check_state_change(new_state_req)) {
				if (new_state_req.id == NERO)
					transition_nero_state(
						new_state_req.state.nero, pdu,
						mc, mpu);
				else if (new_state_req.id == FUNCTIONAL)
					transition_functional_state(
						new_state_req.state.functional,
						pdu, mc, mpu);
			}
		}

		if (!is_ts_rising && get_tsms()) {
			is_ts_rising = true;
			osTimerStart(ts_rising_timer, TS_RISING_BLOCK_TIMEOUT);
		} else if (!get_tsms()) {
			is_ts_rising = false;
			enter_drive_enabled = false;
		}

		// send nero data periodically
		send_nero_msg(mc);
	}
}
