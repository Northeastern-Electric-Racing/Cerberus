#include "steeringio.h"
#include "can.h"
#include "cerberus_conf.h"
#include "cmsis_os.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "state_machine.h"
#include "serial_monitor.h"
#include "nero.h"
#include "stdio.h"
#include "cerb_utils.h"
#include "pedals.h"

#define CAN_QUEUE_SIZE 5 /* messages */

static osMutexAttr_t steeringio_data_mutex_attributes;
static osMutexAttr_t steeringio_ringbuffer_mutex_attributes;

static void debounce_cb(void *arg);

enum { NOT_PRESSED, PRESSED };

typedef struct debounce_cb_args_t {
	steeringio_t *wheel;
	steeringio_button_t button;
} debounce_cb_args_t;

/* Creates a new Steering Wheel interface */
steeringio_t *steeringio_init()
{
	steeringio_t *steeringio = malloc(sizeof(steeringio_t));
	assert(steeringio);

	/* Create Mutexes */
	steeringio->ringbuffer_mutex =
		osMutexNew(&steeringio_ringbuffer_mutex_attributes);
	assert(steeringio->ringbuffer_mutex);

	steeringio->button_mutex =
		osMutexNew(&steeringio_data_mutex_attributes);
	assert(steeringio->button_mutex);

	/* Create debounce utilities */
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS; i++) {
		steeringio->debounce_timers[i] = malloc(sizeof(nertimer_t));
		assert(steeringio->debounce_timers[i]);
	}
	steeringio->debounce_buffer =
		ringbuffer_create(MAX_STEERING_BUTTONS, sizeof(uint8_t));
	assert(steeringio->debounce_buffer);

	return steeringio;
}

bool get_steeringio_button(steeringio_t *wheel, steeringio_button_t button)
{
	if (!wheel)
		return 0;

	bool ret = 0;

	osMutexAcquire(wheel->button_mutex, osWaitForever);
	ret = wheel->debounced_buttons[button];
	osMutexRelease(wheel->button_mutex);

	return ret;
}

static void paddle_left_cb()
{
	if (get_func_state() == F_EFFICIENCY) {
		increase_torque_limit();
	}
}

static void paddle_right_cb()
{
	if (get_func_state() == F_EFFICIENCY) {
		decrease_torque_limit();
	}
}

/* Callback to see if we have successfully debounced */
static void debounce_cb(void *arg)
{
	debounce_cb_args_t *args = (debounce_cb_args_t *)arg;
	if (!args)
		return;

	steeringio_button_t button = args->button;

	if (!button)
		return;

	steeringio_t *wheel = args->wheel;

	if (wheel->raw_buttons[button] && !wheel->debounced[button]) {
		wheel->debounced[button] = true;
		switch (button) {
		case STEERING_PADDLE_LEFT:
			paddle_left_cb();
			break;
		case STEERING_PADDLE_RIGHT:
			paddle_right_cb();
			break;
		case NERO_BUTTON_UP:
			serial_print("Up button pressed \r\n");
			decrement_nero_index();
			break;
		case NERO_BUTTON_DOWN:
			serial_print("Down button pressed \r\n");
			increment_nero_index();
			break;
		case NERO_BUTTON_LEFT:
			// doesnt effect cerb for now
			break;
		case NERO_BUTTON_RIGHT:
			// doesnt effect cerb for now
			break;
		case NERO_BUTTON_SELECT:
			printf("Select button pressed \r\n");
			select_nero_index();
			break;
		case NERO_HOME:
			serial_print("Home button pressed \r\n");
			set_home_mode();
			break;
		default:
			break;
		}
	} else if (!wheel->raw_buttons[button]) {
		/* Button is no longer pressed, so it can be pressed again */
		wheel->debounced[button] = false;
	}
}

/* For updating values via the wheel's CAN message */
void steeringio_update(steeringio_t *wheel, uint8_t button_data)
{
	/* Copy message data to wheelio buffer */
	osMutexAcquire(wheel->button_mutex, osWaitForever);
	/* Data is formatted with each bit within the first byte representing a button and the first two bits of the second byte representing the paddle shifters */

	/* Update raw buttons */
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS - 2; i++) {
		wheel->raw_buttons[i] = (button_data >> i) & 0x01;
	}

	/* Update raw paddle shifters */
	wheel->raw_buttons[STEERING_PADDLE_LEFT] =
		NOT_PRESSED; //(wheel_data[1] >> 0) & 0x01;
	wheel->raw_buttons[STEERING_PADDLE_RIGHT] =
		NOT_PRESSED; //(wheel_data[1] >> 1) & 0x01;

	/* Debounce buttons */
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS; i++) {
		debounce_cb_args_t *debounce_args =
			malloc(sizeof(debounce_cb_args_t));
		debounce_args->wheel = wheel;
		debounce_args->button = i;

		if (wheel->raw_buttons[i])
			debounce(wheel->raw_buttons[i],
				 wheel->debounce_timers[i],
				 STEERING_WHEEL_DEBOUNCE, &debounce_cb,
				 debounce_args);

		/* Debounce that the button has been unpressed */
		else
			debounce(!wheel->raw_buttons[i],
				 wheel->debounce_timers[i],
				 STEERING_WHEEL_DEBOUNCE, &debounce_cb,
				 debounce_args);

		free(debounce_args);
	}
	osMutexRelease(wheel->button_mutex);
}
