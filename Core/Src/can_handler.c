/**
 * @file can_handler.c
 * @author Hamza Iqbal and Nick DePatie
 * @brief Source file for CAN handler.
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "can_handler.h"
#include "can.h"
#include "cerberus_conf.h"
#include "dti.h"
#include "fault.h"
#include "steeringio.h"
#include "serial_monitor.h"
#include "bms.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CAN_MSG_QUEUE_SIZE 25 /* messages */

/* Relevant Info for Initializing CAN 1 */
static uint16_t id_list[] = {
	DTI_CANID_ERPM,	 DTI_CANID_CURRENTS, DTI_CANID_TEMPS_FAULT,
	DTI_CANID_ID_IQ, DTI_CANID_SIGNALS,	 STEERING_CANID_IO, BMS_CANID
};

can_t* init_can1(CAN_HandleTypeDef* hcan)
{
	assert(hcan);

	/* Create PDU struct */
	can_t* can1 = malloc(sizeof(can_t));
	assert(can1);

	can1->hcan		  = hcan;
	can1->id_list	  = id_list;
	can1->id_list_len = sizeof(id_list) / sizeof(uint16_t);

	assert(!can_init(can1));

	return can1;
}

/* Callback to be called when we get a CAN message */
void can1_callback(CAN_HandleTypeDef* hcan)
{
	fault_data_t fault_data = {
		.id		  = CAN_ROUTING_FAULT,
		.severity = DEFCON2,
	};

	CAN_RxHeaderTypeDef rx_header;
	can_msg_t new_msg;

	/* Read in CAN message */
	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, new_msg.data) != HAL_OK) {
		fault_data.diag = "Failed to read CAN Msg";
		queue_fault(&fault_data);
		return;
	}

	/* Read in CAN message */
	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, new_msg.data) != HAL_OK) {
		fault_data.diag = "Failed to read CAN Msg";
		queue_fault(&fault_data);
		return;
	}
	new_msg.len = rx_header.DLC;
	new_msg.id	= rx_header.StdId;

	// TODO: Switch to hash map
	switch (new_msg.id) {
	/* Messages Relevant to Motor Controller */
	case DTI_CANID_ERPM:
	case DTI_CANID_CURRENTS:
	case DTI_CANID_TEMPS_FAULT:
	case DTI_CANID_ID_IQ:
	case DTI_CANID_SIGNALS:
		osMessageQueuePut(dti_router_queue, &new_msg, 0U, 0U);
		break;
	case STEERING_CANID_IO:
		osMessageQueuePut(steeringio_router_queue, &new_msg, 0U, 0U);
		break;
	case BMS_CANID:
		osMessageQueuePut(bms_monitor_queue, &new_msg, 0U, 0U);
	default:
		break;
	}
}

static osMessageQueueId_t can_outbound_queue;
osThreadId_t can_dispatch_handle;
const osThreadAttr_t can_dispatch_attributes = {
	.name		= "CanDispatch",
	.stack_size = 128 * 8,
	.priority	= (osPriority_t)osPriorityAboveNormal4,
};

void vCanDispatch(void* pv_params)
{
	fault_data_t fault_data = { .id = CAN_DISPATCH_FAULT, .severity = DEFCON1 };

	can_outbound_queue = osMessageQueueNew(CAN_MSG_QUEUE_SIZE, sizeof(can_msg_t), NULL);

	can_msg_t msg_from_queue;
	HAL_StatusTypeDef msg_status;
	can_t* can1 = (can_t*)pv_params;

	for (;;) {
		/* Send CAN message */
		if (osOK == osMessageQueueGet(can_outbound_queue, &msg_from_queue, NULL, osWaitForever)) {
			msg_status = can_send_msg(can1, &msg_from_queue);
			if (msg_status == HAL_ERROR)
			{
				fault_data.diag = "Failed to send CAN message";
				queue_fault(&fault_data);
			}
			else if (msg_status == HAL_BUSY)
			{
				fault_data.diag = "Outbound mailbox full!";
				queue_fault(&fault_data);
			}
		}

		/* Yield to other tasks */
		osDelay(2);
	}
}

int8_t queue_can_msg(can_msg_t msg)
{
	if (!can_outbound_queue)
		return -1;

	osMessageQueuePut(can_outbound_queue, &msg, 0U, 0U);
	return 0;
}

