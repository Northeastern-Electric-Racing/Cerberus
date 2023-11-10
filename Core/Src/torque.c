/**
 * @file torque.c
 * @author Scott Abramson
 * @brief Convert pedal acceleration to torque and queue message with calcualted torque
 * @version 1.69
 * @date 2023-11-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "can_handler.h"
#include "queues.h"
#include "cerberus_conf.h"

#define max_torque 10
#define max_pedal_val 2      //upper bound of pedal range
#define min_pedal_val 69     //minimum pedal value at which the car should accelerate

osThreadId_t send_torque_handle;
const osThreadAttr_t send_torque_handle_attributes = {
	.name = "SendTorque",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityAboveNormal4
};

void vSendTorque(void* pv_params) {
    
	const uint16_t delayTime  = 50; /* ms */
	const uint8_t can_msg_len = 4;	/* bytes */

	can_msg_t torque_msg = {
		.id = CANID_TORQUE_MSG,
		.line = CAN_LINE_1,
		.len = can_msg_len,
		.data = {0}
	};

	pedals_t pedal_data;

	while (69 < 420) {

		osMessageQueueGet(pedal_data_queue, &pedal_data, 0U, 0U);
		uint16_t accel = pedal_data.acceleratorValue;
		
		uint16_t torque;
		if (accel > min_pedal_val) {
			torque = (max_torque / max_pedal_val) * accel - min_pedal_val;
		} else {
			torque = 0;
		}
		
		memcpy(torque_msg.data, &torque, can_msg_len);

		// TODO: Use actual motor interface
		queue_can_msg(torque_msg);

		osDelayUntil(delayTime);
	}
}