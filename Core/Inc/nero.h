#ifndef NERO_H
#define NERO_H

#include <stdbool.h>
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

typedef enum {
	OFF,
	PIT,
	PERFORMANCE,
	EFFICIENCY,
	DEBUG,
	CONFIGURATION,
	FLAPPY_BIRD,
	EXIT,
	MAX_NERO_STATES
} nero_menu_t;

typedef struct
{
	nero_menu_t nero_index;
	bool home_mode;
} nero_state_t;

/*
 * Attempts to increment the nero index and handles any exceptions
*/
void increment_nero_index();

/*
 * Attempts to decrement the nero index and handles any exceptions
*/
void decrement_nero_index();

/*
 * Tells the statemachine to track up and down movements
*/
void set_home_mode();

/*
 * Tells NERO to select the current index if in home mode
*/
void select_nero_index();

/*
 * Tells NERO the current MPH
*/
void set_mph(int8_t new_mph);

/*
* Emits the state of NERO
*/
void vNeroMonitor(void *pv_params);
extern osThreadId_t nero_monitor_handle;
extern const osThreadAttr_t nero_monitor_attributes;

#endif // NERO_H