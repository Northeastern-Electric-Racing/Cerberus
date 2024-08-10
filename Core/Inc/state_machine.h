#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "cmsis_os.h"
#include "fault.h"
#include "nero.h"
#include "pdu.h"
#include "dti.h"

/*
 * This is a hierarchical state machine, with "drive modes"
 * being sub states of the ACTIVE functional state
 */
typedef enum {
	READY,
	/* F means functional */
	F_PIT,
	F_PERFORMANCE,
	F_EFFICIENCY,
	REVERSE,
	FAULTED,
	MAX_FUNC_STATES
} func_state_t;

typedef enum {
	OFF,
	PIT, //SPEED_LIMITIED
	PERFORMANCE, //AUTOCROSS
	EFFICIENCY, //ENDURANCE
	DEBUG,
	CONFIGURATION,
	FLAPPY_BIRD,
	EXIT,
	MAX_NERO_STATES
} nero_menu_t;

typedef struct {
	nero_menu_t nero_index;
	bool home_mode;
} nero_state_t;

typedef struct {
	func_state_t functional;
	nero_state_t nero;
} state_t;

extern osThreadId_t sm_director_handle;
extern const osThreadAttr_t sm_director_attributes;

typedef struct {
	pdu_t *pdu;
	dti_t *mc;
} sm_director_args_t;

void vStateMachineDirector(void *pv_params);

/* Retrieves the current functional state */
func_state_t get_func_state();

/**
 * @brief Returns true if car is in active state (pit, performance, efficiency)
 * 
 * @return Whether or not the car is in an active state.
 */
bool get_active();

/*
 * Retrieves the current nero state
 */
nero_state_t get_nero_state();

/**
 * Increments the nero index in the order of nero_menu_t which will be used to select a drive mode
 */
int increment_nero_index();

/**
 * Decrements the nero index int the order of nero_menu_t which will be used to select a drive mode
 */
int decrement_nero_index();

/**
 * Transitions out of home mode, therefore into a drive mode based on the
 * current nero index and mapped using the map_nero_index_to_drive_state function
 * Wil set the functional state to active
 */
int select_nero_index();

/**
 * Sets the home mode to be true, which will turn off the car and set the functional state to ready
 */
int set_home_mode();

/**
 * Handles a fault, setting the functional state to FAULTED
 */
int fault();

#endif