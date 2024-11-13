#ifndef STEERING_H
#define STEERING_H

#include "can.h"
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

/**
 * @brief Update the status of the steering wheel buttons.
 *
 * @param can_msg can message containing data update
 */
void steeringio_update(can_msg_t can_msg);

#endif /* STEERING_H */
