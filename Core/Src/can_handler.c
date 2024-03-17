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
#include "timer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CAN_MSG_QUEUE_SIZE 	25 /* messages */
#define CAN_TEST_MSG		0x069 /* CAN TEST MSG */
#define CAN_BMS_MONITOR		0x070 /*BMS MONITOR WATCHDOG*/ /*Arbitrary*/

#define BMS_WATCHDOG_DURATION 10 /*Duration of btween petting bms monitor watchdog*/


/* Relevant Info for Initializing CAN 1 */
static uint16_t id_list[] = {
	DTI_CANID_ERPM,	 DTI_CANID_CURRENTS, DTI_CANID_TEMPS_FAULT,
	DTI_CANID_ID_IQ, DTI_CANID_SIGNALS,	 STEERING_CANID_IO, CAN_TEST_MSG,
	CAN_BMS_MONITOR
};

void can1_callback(CAN_HandleTypeDef* hcan);

can_t* init_can1(CAN_HandleTypeDef* hcan)
{
	assert(hcan);

	/* Create PDU struct */
	can_t* can1 = malloc(sizeof(can_t));
	assert(can1);

	can1->hcan		  = hcan;
	can1->callback	  = can1_callback;
	can1->id_list	  = id_list;
	can1->id_list_len = sizeof(id_list) / sizeof(uint16_t);

	assert(can_init(can1));

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
	case CAN_TEST_MSG:
		serial_print("UR MOM \n");
	case CAN_BMS_MONITOR:
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
			if (msg_status == HAL_ERROR) {
				fault_data.diag = "Failed to send CAN message";
				queue_fault(&fault_data);
			} else if (msg_status == HAL_BUSY) {
				fault_data.diag = "Outbound mailbox full!";
				queue_fault(&fault_data);
			} else{
				fault_data.diag = "Bing Bong";
				queue_fault(&fault_data);
			}
		}
		/* Uncomment this if needed for debugging */
		// CAN_RxHeaderTypeDef rx_header;
		// can_msg_t new_msg;
		// if(HAL_CAN_GetRxMessage(can1->hcan, CAN_RX_FIFO0, &rx_header, new_msg.data) != HAL_OK)
		// {
		// 	serial_print("IM SCARED \r\n");
		// }
		// else {
		// 	serial_print("MESSAGE CONTENTS\r\nHeader\t%X\r\nData\t%X%X%X%X\r\n", rx_header.StdId, new_msg.data[0], new_msg.data[1], new_msg.data[2], new_msg.data[3]);
		// }

		/* Yield to other tasks */
		osDelay(CAN_DISPATCH_DELAY);
	}
}

osThreadId_t bms_can_monitor_handle;
const osThreadAttr_t bms_can_monitor_attributes = {
	.name = "BMSCANMonitor",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityLow1 /*Adjust priority*/
};

void vBMSCANMonitor(void* pv_params) 
{

	fault_data_t fault_data = { .id = BMS_CAN_MONITOR_FAULT, .severity = DEFCON1 }; /*Ask about severity*/

	bms_can_queue = osMessageQueueNew(CAN_MSG_QUEUE_SIZE, sizeof(can_msg_t), NULL);

	can_msg_t msg_from_queue;
	HAL_StatusTypeDef msg_status;
	can_t* can1 = (can_t*)pv_params;

	nertimer_t timer;
	
	if (osOK == osMessageQueueGet(can_outbound_queue, &msg_from_queue, NULL, osWaitForever)) {
		if (msg_from_queue.id == CAN_BMS_MONITOR) {
			if (!is_timer_active(&timer)) {
				start_timer(&timer, BMS_WATCHDOG_DURATION);
			} else {
				cancel_timer(&timer);
			}
		}
	}

	if (is_timer_expired(&timer)) {
		fault_data.diag = "Failing To Receive CAN Messages from Sheperd";
		queue_fault(&fault_data);
	}
}

int8_t queue_can_msg(can_msg_t msg)
{
	if (!can_outbound_queue)
		return -1;

	osMessageQueuePut(can_outbound_queue, &msg, 0U, 0U);
	return 0;
}
