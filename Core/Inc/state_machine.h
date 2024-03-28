#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "cmsis_os.h"
#include "fault.h"

typedef enum { BOOT, READY, DRIVING, FAULTED, MAX_FUNC_STATES } func_state_t;

typedef enum {
	NOT_DRIVING = -1,
	REVERSE,
	PIT,
	EFFICIENCY,
	PERFORMANCE,
	MAX_DRIVE_STATES
} drive_state_t;

extern osThreadId_t sm_director_handle;
extern const osThreadAttr_t sm_director_attributes;

void vStateMachineDirector(void* pv_params);

/* Adds a functional state transition to be processed */
int queue_func_state(func_state_t new_state);

/* Retrieves the current functional state */
func_state_t get_func_state();

/*
 * Adds a drive state transition to be processed
 * Will return negative if functional state is not DRIVING
 */
int queue_drive_state(drive_state_t new_state);

/*
 * Retrieves the current drive state
 * Will return negative if functional state is not DRIVING
 */
drive_state_t get_drive_state();

#endif