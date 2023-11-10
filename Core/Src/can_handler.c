/**
 * @file can_handler.c
 * @author Hamza Iqbal
 * @brief Source file for CAN handler.
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "can_handler.h"
#include "can.h"
#include "can_config.h"
#include <stdlib.h>
#include "fault.h"
#include <string.h>
#include "cerberus_conf.h"

#define CAN_MSG_QUEUE_SIZE 25 /* messages */
#define NUM_CALLBACKS 5 // Update when adding new callbacks

static osMessageQueueId_t can_inbound_queue;
osThreadId_t route_can_incoming_handle;
const osThreadAttr_t route_can_incoming_attributes = {
	.name = "RouteCanIncoming",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityAboveNormal4
};

static CAN_HandleTypeDef* hcan1;

typedef void (*callback_t)(can_msg_t);

/* Struct to couple function with message IDs */
typedef struct
{
	uint8_t id;
	callback_t function;
} function_info_t;

// TODO: Evaluate memory usage here
static function_info_t can_callbacks[] = {
	// TODO: Implement MC_Update and other callbacks
	//{ .id = 0x2010, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2110, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2210, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2310, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2410, .function = (*MC_update)(can_msg_t) }
};

static callback_t getFunction(uint8_t id)
{
	int i;

	for (i = 0; i < NUM_CALLBACKS; i++) {
		if (can_callbacks[i].id == id)
			return can_callbacks[i].function;
	}
	return NULL;
}

void can1_isr()
{
	// TODO: Wrap this HAL function into a "get_message" function in the CAN driver
	CAN_RxHeaderTypeDef rx_header;
	can_msg_t new_msg;
	new_msg.line = CAN_LINE_1;
	HAL_CAN_GetRxMessage(hcan1, CAN_RX_FIFO1, &rx_header, new_msg.data);
	new_msg.len = rx_header.DLC;
	new_msg.id = rx_header.StdId;

	/* Publish to Onboard Temp Queue */
	osMessageQueuePut(can_inbound_queue, &new_msg, 0U, 0U);
}

void vRouteCanIncoming(void* pv_params)
{
	can_msg_t* message;
	osStatus_t status;
	callback_t callback;

	hcan1 = (CAN_HandleTypeDef*)pv_params;

	// TODO: Initialize CAN here? Or at least initialize it somewhere

	can_inbound_queue = osMessageQueueNew(CAN_MSG_QUEUE_SIZE, sizeof(can_msg_t), NULL);

	// TODO: Link CAN1_ISR via hcan1, future ticket needs to enable CAN driver to pass in devoloper ISR

	for (;;) {
		/* Wait until new CAN message comes into queue */
		status = osMessageQueueGet(can_inbound_queue, &message, NULL, 0U);
		if (status != osOK) {
			// TODO: Trigger fault ?
		} else {
			callback = getFunction(message->id);
			if (callback == NULL) {
				// TODO: Trigger low priority error
			} else {
				callback(*message);
			}
		}
	}
}

static osMessageQueueId_t can_outbound_queue = 0;
osThreadId_t can_dispatch_handle;
const osThreadAttr_t can_dispatch_attributes = {
	.name = "CanDispatch",
	.stack_size = 128 * 8,
	.priority = (osPriority_t) osPriorityAboveNormal4,
};

void vCanDispatch(void* pv_params) {

	const uint16_t can_dispatch_delay = 100; //ms
	fault_data_t fault_data = {
		.id = 1, /* this is arbitrary */
		.severity = DEFCON2
	};

	can_outbound_queue = osMessageQueueNew(CAN_MSG_QUEUE_SIZE, sizeof(can_msg_t), NULL);

	can_msg_t msg_from_queue;

	// Wait until a message is in the queue, send messages when they are in the queue
	for(;;) {
		// "borrowed" from temp sensor CAN code (PR 58)
		/* Send CAN message */         //queue id        buffer to put data     timeout
		if (osOK == osMessageQueueGet(can_outbound_queue, &msg_from_queue, NULL, 50)) {
			if (can_send_message(msg_from_queue)) {
				fault_data.diag = "Failed to send CAN message";
				osMessageQueuePut(fault_handle_queue, &fault_data , 0U, 0U);
			}
		}

		/* Yield to other tasks */
		osDelayUntil(can_dispatch_delay);
	}
}

int8_t queue_can_msg(can_msg_t msg) {
	
	fault_data_t fault_data = {
		.id = 2, /* this is arbitrary */
		.severity = DEFCON4
	};

	if(!can_outbound_queue) {
		return 1;
	}

	if(osOK == osMessageQueuePut(can_outbound_queue, &msg, 0U, 0U)) {
		return 0;
	}

	fault_data.diag = "Failed to put CAN message in queue";
	osMessageQueuePut(fault_handle_queue, &fault_data , 0U, 0U);
	return 1;
}