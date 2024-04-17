#ifndef STEERING_H
#define STEERING_H

#include "cmsis_os.h"
#include "ringbuffer.h"
#include <stdbool.h>
#include <stdint.h>

#define STEERING_CANID_IO 0x680

typedef enum {
	NONE,
	NERO_BUTTON_DOWN,
	NERO_BUTTON_SELECT,
	NERO_BUTTON_RIGHT,
	NERO_HOME,
	NERO_BUTTON_UP,
	NERO_BUTTON_LEFT,
	STEERING_PADDLE_RIGHT,
	STEERING_PADDLE_LEFT,
	MAX_STEERING_BUTTONS
} steeringio_button_t;

typedef struct {
	osMutexId_t* button_mutex; /* Necessary to allow multiple threads to access same data */
	osMutexId_t* ringbuffer_mutex;
	osTimerId_t debounce_timers[MAX_STEERING_BUTTONS];
	ringbuffer_t* debounce_buffer;
	bool raw_buttons[MAX_STEERING_BUTTONS];
	bool debounced_buttons[MAX_STEERING_BUTTONS];
} steeringio_t;

/* Utilities for Decoding CAN message */
extern osThreadId_t steeringio_router_handle;
extern const osThreadAttr_t steeringio_router_attributes;
extern osMessageQueueId_t steeringio_router_queue;
void vSteeringIORouter(void* pv_params);

/* Creates a new Steering Wheel interface */
steeringio_t* steeringio_init();

bool get_steeringio_button(steeringio_t* wheel, steeringio_button_t button);

/* For updating values via the wheel's CAN message */
void steeringio_update(steeringio_t* wheel, uint8_t wheel_data[], uint8_t len);

#endif /* STEERING_H */
