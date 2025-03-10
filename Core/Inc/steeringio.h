#ifndef STEERING_H
#define STEERING_H

#include "can.h"

#define BUTTON_CANID_IO 0x680
#define DIAL_CANID_IO	0x681

typedef enum {
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_ESC,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_ENTER,
	BUTTON_SPARE,
	MAX_STEERING_BUTTONS
} steeringio_button_t;

typedef enum {
	DIAL_SWITCH_1,
	DIAL_SWITCH_2,
	DIAL_SWITCH_3,
	DIAL_SWITCH_4,
	DIAL_SWITCH_5,
	MAX_DIAL_SIZE,
} steeringio_dial_t;

/**
 * @brief Update the status of the steering wheel buttons.
 *
 * @param can_msg can message containing data update
 */
void buttons_update(can_msg_t can_msg);

/**
 * @brief Update the status of the active steering wheel dial switch.
 *
 * @param can_msg can message containing data update
 */
void dial_update(can_msg_t can_msg);

#endif /* STEERING_H */
