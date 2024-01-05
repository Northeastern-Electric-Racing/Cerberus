#include "steeringio.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "cmsis_os.h"
#include "cerberus_conf.h"
#include "can.h"

#define CAN_QUEUE_SIZE      5   /* messages */

static osMutexAttr_t steeringio_data_mutex_attributes;
static osMutexAttr_t steeringio_ringbuffer_mutex_attributes;
osMessageQueueId_t steeringio_router_queue;

static void debounce_cb(void *arg);

enum {
	NOT_PRESSED,
	PRESSED
};

/* Creates a new Steering Wheel interface */
steeringio_t *steeringio_init()
{
	steeringio_t *steeringio = malloc(sizeof(steeringio_t));
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

/* Callback to see if we have successfully debounced */
static void debounce_cb(void *arg)
{
	steeringio_t *wheel = (steeringio_t *)arg;
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
		wheel->debounced_buttons[button] = PRESSED;
}

/* For updating values via the wheel's CAN message */
void steeringio_update(steeringio_t *wheel, uint8_t wheel_data[], uint8_t len)
{
	if (!wheel || !wheel_data)
		return;

	//TODO: Revisit how new wheel's data is formatted
	/* Copy message data to wheelio buffer */
	//memcpy(&io, wheel_data, len);

	/* If the value was set high and is not being debounced, trigger timer for debounce */
	osMutexAcquire(wheel->button_mutex, osWaitForever);
	for (uint8_t i = 0; i < MAX_STEERING_BUTTONS; i++) {
		if (wheel->debounced_buttons[i] && wheel->raw_buttons[i]) {
			wheel->debounced_buttons[i] = PRESSED;
		}
		else if (wheel->raw_buttons[i] && !osTimerIsRunning(wheel->debounce_timers[i])) {
			osTimerStart(wheel->debounce_timers[i], STEERING_WHEEL_DEBOUNCE);
			ringbuffer_enqueue(wheel->debounce_buffer, &i);
		}
		else {
			wheel->debounced_buttons[i] = NOT_PRESSED;
		}
	}
	osMutexRelease(wheel->button_mutex);
}

/* Inbound Task-specific Info */
osThreadId_t steeringio_router_handle;
const osThreadAttr_t steeringio_router_attributes = {
	.name = "SteeringIORouter",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityNormal3
};

void vSteeringIORouter(void* pv_params)
{
	can_msg_t message;
	osStatus_t status;
	//fault_data_t fault_data = { .id = DTI_ROUTING_FAULT, .severity = DEFCON2 };

	steeringio_t *wheel = (steeringio_t *)pv_params;

	for (;;) {
		/* Wait until new CAN message comes into queue */
		status = osMessageQueueGet(steeringio_router_queue, &message, NULL, osWaitForever);
		if (status == osOK){
			steeringio_update(wheel, message.data, message.len);
		}
	}
}
