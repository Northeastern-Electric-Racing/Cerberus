#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "cmsis_os.h"
#include "fault.h"
#include "nero.h"

/*
 * This is a hierarchical state machine, with "drive modes"
 * being sub states of the ACTIVE functional state
 */
typedef enum { BOOT, READY, ACTIVE, FAULTED, MAX_FUNC_STATES } func_state_t;
typedef enum {
	NOT_DRIVING,
	REVERSE,
	SPEED_LIMITED,
	AUTOCROSS,
	ENDURANCE,
	MAX_DRIVE_STATES
} drive_state_t;

extern osThreadId_t sm_director_handle;
extern const osThreadAttr_t sm_director_attributes;

typedef struct {
	enum { FUNCTIONAL, DRIVE } id;

	union {
		func_state_t functional;
		drive_state_t drive;
	} state;
} state_req_t;

void vStateMachineDirector(void *pv_params);

int queue_state_transition(state_req_t request);

/* Retrieves the current functional state */
func_state_t get_func_state();

/*
 * Retrieves the current drive state
 * Will return negative if functional state is not DRIVING
 */
drive_state_t get_drive_state();

#endif