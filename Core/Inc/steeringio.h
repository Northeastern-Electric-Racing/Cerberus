#ifndef STEERING_H
#define STEERING_H

#include "cmsis_os.h"
#include "ringbuffer.h"
#include "timer.h"
#include <stdbool.h>
#include <stdint.h>

#define STEERING_CANID_IO 0x680

typedef enum {
	NONE,
	NONE2,
	NERO_BUTTON_LEFT,
	NERO_BUTTON_UP,
	NERO_BUTTON_RIGHT,
	NERO_HOME,
	NERO_BUTTON_SELECT,
	NERO_BUTTON_DOWN,
	STEERING_PADDLE_LEFT,
	STEERING_PADDLE_RIGHT,
	MAX_STEERING_BUTTONS
} steeringio_button_t;

typedef struct {
	osMutexId_t *
		button_mutex; /* Necessary to allow multiple threads to access same data */
	osMutexId_t *ringbuffer_mutex;
	nertimer_t *debounce_timers[MAX_STEERING_BUTTONS];
	ringbuffer_t *debounce_buffer;
	bool raw_buttons[MAX_STEERING_BUTTONS];
	bool debounced_buttons[MAX_STEERING_BUTTONS];
	/* Array indicating that a button has already been pressed and not unpressed */
	bool debounced[MAX_STEERING_BUTTONS];
} steeringio_t;

/* Creates a new Steering Wheel interface */
steeringio_t *steeringio_init();

/**
 * @brief Update the status of the steering wheel buttons.
 * 
 * @param wheel Pointer to struct representing the steering wheel
 * @param button_data Unsigned 8 bit integer where each bit is a button status
 */
void steeringio_update(steeringio_t *wheel, uint8_t button_data);

#endif /* STEERING_H */
