#ifndef STEERING_H
#define STEERING_H

#include "can.h"

#define STEERING_CANID_IO 0x680

typedef enum {
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_ESC,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_ENTER,
	MAX_STEERING_BUTTONS
} steeringio_button_t;

/**
 * @brief Update the status of the steering wheel buttons.
 *
 * @param can_msg can message containing data update
 */
void steeringio_update(can_msg_t can_msg);

#endif /* STEERING_H */
