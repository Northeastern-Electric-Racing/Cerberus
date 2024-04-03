#include "steeringio.h"
#include "can.h"
#include "cerberus_conf.h"
#include "cmsis_os.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "state_machine.h"
#include "serial_monitor.h"
#include "nero_state_machine.h"
#include "stdio.h"

#define CAN_QUEUE_SIZE 5 /* messages */

static osMutexAttr_t steeringio_data_mutex_attributes;
static osMutexAttr_t steeringio_ringbuffer_mutex_attributes;
osMessageQueueId_t steeringio_router_queue;

static void debounce_cb(void* arg);

enum { NOT_PRESSED, PRESSED };

/* Creates a new Steering Wheel interface */
steeringio_t* steeringio_init()
{
	steeringio_t* steeringio = malloc(sizeof(steeringio_t));
	assert(steeringio);

	/* Create Mutexes */
	steeringio->ringbuffer_mutex = osMutexNew(&steeringio_data_mutex_attributes);
	assert(steeringio->ringbuffer_mutex);

	steeringio->button_mutex = osMutexNew(&steeringio_ringbuffer_mutex_attributes);
	assert(steeringio->button_mutex);

	/* Create debounce utilities */
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS; i++) {
		steeringio->debounce_timers[i] = osTimerNew(debounce_cb, osTimerOnce, steeringio, NULL);
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
static void debounce_cb(void* arg)
{
	steeringio_t* wheel = (steeringio_t*)arg;
	if (!wheel)
		return;

	steeringio_button_t button;

	/*
	 * Pop off most recent button being debounced
	 * Note: timers are started sequentially, therefore buttons
	 *  will be popped off in order
	 *
	 * Also needs to be atomic
	 */
	osMutexAcquire(wheel->ringbuffer_mutex, osWaitForever);
	ringbuffer_dequeue(wheel->debounce_buffer, &button);
	osMutexRelease(wheel->ringbuffer_mutex);

	if (!button)
		return;

	if (wheel->raw_buttons[button])
		wheel->debounced_buttons[button] = NOT_PRESSED;
}

/* For updating values via the wheel's CAN message */
void steeringio_update(steeringio_t* wheel, uint8_t wheel_data[], uint8_t len)
{
	if (!wheel || !wheel_data)
		return;

	// TODO: Revisit how new wheel's data is formatted
	/* Copy message data to wheelio buffer */
	osMutexAcquire(wheel->button_mutex, osWaitForever);
	// Data is formatted with each bit within the first byte representing a button and the first two bits of the second byte representing the paddle shifters

	/* Update raw buttons */
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS - 2; i++) {
		wheel->raw_buttons[i] = (wheel_data[0] >> i) & 0x01;
	}

	/* Update raw paddle shifters */
	wheel->raw_buttons[STEERING_PADDLE_LEFT] = (wheel_data[1] >> 0) & 0x01;
	wheel->raw_buttons[STEERING_PADDLE_RIGHT] = (wheel_data[1] >> 1) & 0x01;

	/* If the value was set high and is not being debounced, trigger timer for debounce */
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS; i++) {
		if (!wheel->debounced_buttons[i] && wheel->raw_buttons[i]) {
			wheel->debounced_buttons[i] = PRESSED;
			printf("%d index pressed \r\n",i);
			switch (i) {
				case STEERING_PADDLE_LEFT:
					// doesnt effect cerb for now
					break;
				case STEERING_PADDLE_RIGHT:
					// doesnt effect cerb for now
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
					printf("Home button pressed \r\n");
					set_home_mode();
					break;
				default:
					break;
			}
		} else if (wheel->raw_buttons[i] && !osTimerIsRunning(wheel->debounce_timers[i])) {
			// osTimerStart(wheel->debounce_timers[i], STEERING_WHEEL_DEBOUNCE);
			// ringbuffer_enqueue(wheel->debounce_buffer, &i);
		} else {
			wheel->debounced_buttons[i] = NOT_PRESSED;
		}
	}
	osMutexRelease(wheel->button_mutex);
}

/* Inbound Task-specific Info */
osThreadId_t steeringio_router_handle;
const osThreadAttr_t steeringio_router_attributes = { .name		  = "SteeringIORouter",
													  .stack_size = 128 * 8,
													  .priority = (osPriority_t)osPriorityNormal4 };

void vSteeringIORouter(void* pv_params)
{
	can_msg_t message;
	osStatus_t status;
	// fault_data_t fault_data = { .id = STEERINGIO_ROUTING_FAULT, .severity = DEFCON2 };

	steeringio_t *wheel = (steeringio_t *)pv_params;

	for (;;) {
		/* Wait until new CAN message comes into queue */
		status = osMessageQueueGet(steeringio_router_queue, &message, NULL, osWaitForever);
		if (status == osOK){
			steeringio_update(wheel, message.data, message.len);
		}
		osThreadYield();
	}
}
