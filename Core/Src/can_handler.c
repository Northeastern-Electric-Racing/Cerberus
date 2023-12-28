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
#include <stdlib.h>
#include "fault.h"
#include <string.h>
#include "cerberus_conf.h"

#define CAN_MSG_QUEUE_SIZE 25 /* messages */
#define NUM_INBOUND_CAN_IDS 1 /* Update when adding new callbacks */

/* Queue for Inbound CAN Messages */
static osMessageQueueId_t can_inbound_queue;

/* Relevant Info for Initializing CAN 1 */
static can_t *can1;

static const uint16_t id_list[NUM_INBOUND_CAN_IDS] = {
	//CANID_X,
	NULL
};

/* Relevant Info for Cerberus CAN LUT */
typedef void (*callback_t)(can_msg_t);

typedef struct
{
	uint8_t id;
	callback_t function;
} function_info_t;

static function_info_t can_callbacks[NUM_INBOUND_CAN_IDS] = {
	// TODO: Implement MC_Update and other callbacks
	//{ .id = 0x2010, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2110, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2210, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2310, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2410, .function = (*MC_update)(can_msg_t) }
};

static callback_t getFunction(uint8_t id)
{
	//TODO: optimization of create algo to more efficiently find handler
	for (uint8_t i = 0; i < NUM_INBOUND_CAN_IDS; i++) {
		if (can_callbacks[i].id == id)
			return can_callbacks[i].function;
	}
	return NULL;
}

/* Callback to be called when we get a CAN message */
void can1_callback(CAN_HandleTypeDef *hcan)
{
	fault_data_t fault_data = {
		.id = CAN_ROUTING_FAULT,
		.severity = DEFCON2,
	};

	CAN_RxHeaderTypeDef rx_header;

	can_msg_t new_msg;

	/* Read in CAN message */
	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, new_msg.data) != HAL_OK) {
        fault_data.diag = "Failed to read CAN Msg";
		queue_fault(&fault_data);
    }
	new_msg.len = rx_header.DLC;
	new_msg.id = rx_header.StdId;

	/* Publish to CAN processing queue */
	osMessageQueuePut(can_inbound_queue, &new_msg, 0U, 0U);
}

/* Inbound Task-specific Info */
osThreadId_t route_can_incoming_handle;
const osThreadAttr_t route_can_incoming_attributes = {
	.name = "RouteCanIncoming",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityAboveNormal4
};

void vRouteCanIncoming(void* pv_params)
{
	can_msg_t* message;
	osStatus_t status;
	callback_t callback;
	fault_data_t fault_data = { .id = CAN_ROUTING_FAULT, .severity = DEFCON2 };
	can1->callback = can1_callback;
	can1->id_list = id_list;
	can1->id_list_len = NUM_INBOUND_CAN_IDS;

	can1->hcan = (CAN_HandleTypeDef *)pv_params;

	if (can_init(can1)) {
		fault_data.diag = "Failed to init CAN handler";
		queue_fault(&fault_data);
	}

	can_inbound_queue = osMessageQueueNew(CAN_MSG_QUEUE_SIZE, sizeof(can_msg_t), NULL);

	for (;;) {
		/* Wait until new CAN message comes into queue */
		status = osMessageQueueGet(can_inbound_queue, &message, NULL, 0U);
		if (status != osOK) {
			fault_data.diag = "CAN Router Init Failed";
			queue_fault(&fault_data);
		} else {
			callback = getFunction(message->id);
			if (callback == NULL) {
				fault_data.diag = "No callback found";
				queue_fault(&fault_data);
			} else {
				callback(*message);
			}
		}
	}
}

static osMessageQueueId_t can_outbound_queue;
osThreadId_t can_dispatch_handle;
const osThreadAttr_t can_dispatch_attributes = {
	.name = "CanDispatch",
	.stack_size = 128 * 8,
	.priority = (osPriority_t) osPriorityAboveNormal4,
};

void vCanDispatch(void* pv_params) {

	const uint16_t can_dispatch_delay = 100; //ms
	fault_data_t fault_data = {
		.id = CAN_DISPATCH_FAULT,
		.severity = DEFCON1
	};

	can_outbound_queue = osMessageQueueNew(CAN_MSG_QUEUE_SIZE, sizeof(can_msg_t), NULL);

	can_msg_t msg_from_queue;

	for(;;) {
		/* Send CAN message */
		if (osOK == osMessageQueueGet(can_outbound_queue, &msg_from_queue, NULL, 50)) {
			if (can_send_msg(can1, &msg_from_queue)) {
				fault_data.diag = "Failed to send CAN message";
				queue_fault(&fault_data);
			}
		}

		/* Yield to other tasks */
		osDelayUntil(can_dispatch_delay);
	}
}

int queue_can_msg(can_msg_t msg) 
{
	if(!can_outbound_queue)
		return -1;

	osMessageQueuePut(can_outbound_queue, &msg, 0U, 0U);
}
