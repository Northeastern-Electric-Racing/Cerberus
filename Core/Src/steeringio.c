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
#include "torque.h"

#define CAN_QUEUE_SIZE 5 /* messages */

static osMutexAttr_t steeringio_data_mutex_attributes;
static osMutexAttr_t steeringio_ringbuffer_mutex_attributes;
osMessageQueueId_t steeringio_router_queue;

// static void debounce_cb(void* arg);

enum { NOT_PRESSED, PRESSED };

typedef struct debounce_cb_args_t {
	steeringio_t *wheel;
	steeringio_button_t button;
} debounce_cb_args_t;

/* Creates a new Steering Wheel interface */
steeringio_t* steeringio_init()
{
	steeringio_t* steeringio = malloc(sizeof(steeringio_t));
	assert(steeringio);

	/* Create Mutexes */
	steeringio->ringbuffer_mutex = osMutexNew(&steeringio_ringbuffer_mutex_attributes);
	assert(steeringio->ringbuffer_mutex);

	steeringio->button_mutex = osMutexNew(&steeringio_data_mutex_attributes);
	assert(steeringio->button_mutex);

	/* Create debounce utilities */
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS; i++) {
		debounce_cb_args_t *debounce_args = malloc(sizeof(debounce_cb_args_t));
		debounce_args->wheel = steeringio;
		debounce_args->button = i;
		nertimer_t timer;
		steeringio->debounce_timers[i] = &timer;
		assert(steeringio->debounce_timers[i]);
	}
	steeringio->debounce_buffer = ringbuffer_create(MAX_STEERING_BUTTONS, sizeof(uint8_t));
	assert(steeringio->debounce_buffer);

	/* Create Queue for CAN signaling */
	steeringio_router_queue = osMessageQueueNew(CAN_QUEUE_SIZE, sizeof(can_msg_t), NULL);
	assert(steeringio_router_queue);

	return steeringio;
}

bool get_steeringio_button(steeringio_t* wheel, steeringio_button_t button)
{
	if (!wheel)
		return 0;

	bool ret = 0;

	osMutexAcquire(wheel->button_mutex, osWaitForever);
	ret = wheel->debounced_buttons[button];
	osMutexRelease(wheel->button_mutex);

	return ret;
}

/* Callback to see if we have successfully debounced */
// static void debounce_cb(void* arg)
// {
// 	debounce_cb_args_t* args = (debounce_cb_args_t*)arg;
// 	if (!args)
// 		return;

// 	steeringio_button_t button = args->button;

// 	if (!button)
// 		return;

// 	steeringio_t *wheel = args->wheel;

// 	if (wheel->raw_buttons[button])
// 		wheel->debounced_buttons[button] = PRESSED;
// }

static void paddle_left_cb() {
	if (get_drive_state() == ENDURANCE) {
		increase_torque_limit();
	}
}

static void paddle_right_cb() {
	if (get_drive_state() == ENDURANCE) {
		decrease_torque_limit();
	}
}

/* For updating values via the wheel's CAN message */
void steeringio_update(steeringio_t* wheel, uint8_t wheel_data[])
{

	/* Copy message data to wheelio buffer */
	osMutexAcquire(wheel->button_mutex, osWaitForever);
	// Data is formatted with each bit within the first byte representing a button and the first two bits of the second byte representing the paddle shifters

	/* Update raw buttons */
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS - 2; i++) {
		wheel->raw_buttons[i] = (wheel_data[0] >> i) & 0x01;
	}

	/* Update raw paddle shifters */

	wheel->raw_buttons[STEERING_PADDLE_LEFT] = NOT_PRESSED; //(wheel_data[1] >> 0) & 0x01;
	wheel->raw_buttons[STEERING_PADDLE_RIGHT] = NOT_PRESSED; //(wheel_data[1] >> 1) & 0x01;

	/* If the value was set high and is not being debounced, trigger timer for debounce */
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS; i++) {
		if (is_timer_expired(wheel->debounce_timers[i]) && wheel->raw_buttons[i]) {
			wheel->debounced_buttons[i] = PRESSED;
			serial_print("%d index pressed \r\n",i);
			switch (i) {
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
		} else if (wheel->raw_buttons[i] && !is_timer_active(wheel->debounce_timers[i])) {
			start_timer(wheel->debounce_timers[i], STEERING_WHEEL_DEBOUNCE);
		} else if (wheel->raw_buttons[i] && is_timer_active(wheel->debounce_timers[i])) {
			continue;
		} else if (!wheel->raw_buttons[i] && is_timer_active(wheel->debounce_timers[i])) {
			wheel->debounced_buttons[i] = NOT_PRESSED;
			cancel_timer(wheel->debounce_timers[i]);
		}  else {
			serial_print("Scott sucks balls at coding\r\n");
		}
	}
	osMutexRelease(wheel->button_mutex);
}

/* Inbound Task-specific Info */
osThreadId_t steeringio_router_handle;
const osThreadAttr_t steeringio_router_attributes = { .name		  = "SteeringIORouter",
													  .stack_size = 128 * 8,
													  .priority = (osPriority_t)osPriorityHigh1 };

void vSteeringIORouter(void* pv_params)
{
	button_data_t message;
	osStatus_t status;
	// fault_data_t fault_data = { .id = STEERINGIO_ROUTING_FAULT, .severity = DEFCON2 };

	steeringio_t *wheel = (steeringio_t *)pv_params;

	for (;;) {
		/* Wait until new CAN message comes into queue */
		status = osMessageQueueGet(steeringio_router_queue, &message, NULL, osWaitForever);
		if (status == osOK){
			printf("joe mama");\
			steeringio_update(wheel, message.data);
		}
	}
}
