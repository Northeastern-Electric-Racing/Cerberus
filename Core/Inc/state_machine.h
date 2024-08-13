#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "cmsis_os.h"
#include "fault.h"
#include "nero.h"
#include "pdu.h"
#include "dti.h"

/**
 * @brief Enum defining the functional states of the car.
 * 
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

/**
 * @brief Emum that maps to NERO indexes to the menu on the NERO screen.
 * 
 */
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

/**
 * @brief Struct for defining the state of the car.
 * 
 */
typedef struct {
	func_state_t functional;
	nero_state_t nero;
} state_t;

typedef struct {
	pdu_t *pdu;
	dti_t *mc;
} sm_director_args_t;

/**
 * @brief Task for handling state transitions.
 * 
 * @param pv_params Pointer to sm_director_args_t.
 */
void vStateMachineDirector(void *pv_params);
extern osThreadId_t sm_director_handle;
extern const osThreadAttr_t sm_director_attributes;

/**
 * @brief Retrieve the current functional state.
 * 
 * @return func_state_t Struct containing the current functional state
 */
func_state_t get_func_state();

/**
 * @brief Returns true if car is in active state (pit, performance, efficiency)
 * 
 * @return Whether or not the car is in an active state.
 */
bool get_active();

/**
 * @brief Retrieves the current NERO state.
 * 
 * @return nero_state_t The current NERO state
 */
nero_state_t get_nero_state();

/**
 * @brief Increments the nero index in the order of nero_menu_t which will be used to select a drive mode.
 * 
 * @return int Error code resulting from queueing a state transition
 */
int increment_nero_index();

/**
 * @brief Decrements the nero index int the order of nero_menu_t which will be used to select a drive mode.
 * 
 * @return int Error code resulting from queueing a state transition
 */
int decrement_nero_index();

/**
 * @brief Enter the mode defined by the NERO index.
 * 
 * @return int Error code resulting from queueing a state transition
 */
int select_nero_index();

/**
 * Sets the home mode to be true, which will turn off the car and set the functional state to ready
 */

/**
 * @brief Queue a state transition to enter home mode.
 * 
 * @return int Error code resulting from queueing a state transition
 */
int set_home_mode();

/**
 * @brief Queue a state transition to set the functinoal mode of the car to the faulted state.
 * 
 * @return int Error code resulting from queueing a state transition
 */
int fault();

#endif