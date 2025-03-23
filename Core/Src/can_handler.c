#include "can_handler.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "rtos_utils.h"
#include "fault.h"
#include "steeringio.h"
#include "control.h"
#include "bms.h"

#define CAN_MSG_QUEUE_SIZE 50 /* messages */

#define CAN_DISPATCH_FLAG 1U

#define NEW_CAN_MSG_FLAG 1U

static osMessageQueueId_t can_outbound_queue;
static osMessageQueueId_t can_inbound_queue;

can_t *can1;

/* Relevant Info for Initializing CAN 1 */
static uint16_t id_list_1[4] = {
	DTI_CANID_ERPM,
	DTI_CANID_TEMPS_FAULT,
	BMS_DCL_MSG,
	BUTTON_CANID_IO,
};

static uint16_t id_list_2[4] = { DIAL_CANID_IO, CONTROL_CANID_FANBATTBOX,
				 CONTROL_CANID_PUMP, CONTROL_CANID_RADFAN };

void init_can1(CAN_HandleTypeDef *hcan)
{
	assert(hcan);

	/* Create PDU struct */
	can1 = malloc(sizeof(can_t));
	assert(can1);

	can1->hcan = hcan;
	assert(!can_init(can1));
	assert(!can_add_filter_standard(can1, id_list_1));
	assert(!can_add_filter_standard(can1, id_list_2));

	can_outbound_queue =
		osMessageQueueNew(CAN_MSG_QUEUE_SIZE, sizeof(can_msg_t), NULL);
	can_inbound_queue =
		osMessageQueueNew(CAN_MSG_QUEUE_SIZE, sizeof(can_msg_t), NULL);
}

/* Callback to be called when we get a CAN message */
void can1_callback(CAN_HandleTypeDef *hcan)
{
	fault_data_t fault_data = {
		.fault_index.crit_fault = CAN_ROUTING_FAULT,
		.severity = CRITICAL,
	};

	CAN_RxHeaderTypeDef rx_header;
	can_msg_t new_msg;

	/* Read in CAN message */
	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header,
				 new_msg.data) != HAL_OK) {
		fault_data.diag = "Failed to read CAN Msg";
		queue_fault(&fault_data);
		return;
	}

	new_msg.len = rx_header.DLC;

	if (rx_header.IDE == CAN_ID_EXT) {
		// If the message has an extended CAN ID, save the message accordingly.
		new_msg.id = rx_header.ExtId;
		new_msg.id_is_extended = true;
	} else {
		// If the message has a standard CAN ID, save the message accordingly.
		new_msg.id = rx_header.StdId;
		new_msg.id_is_extended = false;
	}

	queue_and_set_flag(can_inbound_queue, &new_msg, can_receive_thread,
			   NEW_CAN_MSG_FLAG);
}

int8_t queue_can_msg(can_msg_t msg)
{
	if (!can_outbound_queue)
		return -1;

	return queue_and_set_flag(can_outbound_queue, &msg, can_dispatch_handle,
				  CAN_DISPATCH_FLAG);
}

osThreadId_t can_dispatch_handle;
const osThreadAttr_t can_dispatch_attributes = {
	.name = "CanDispatch",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime6,
};

void vCanDispatch(void *pv_params)
{
	fault_data_t fault_data = { .fault_index.crit_fault =
					    CAN_DISPATCH_FAULT,
				    .severity = CRITICAL };

	can_msg_t msg_from_queue;
	HAL_StatusTypeDef msg_status;

	CAN_HandleTypeDef *hcan = (CAN_HandleTypeDef *)pv_params;
	assert(hcan);

	for (;;) {
		osThreadFlagsWait(CAN_DISPATCH_FLAG, osFlagsWaitAny,
				  osWaitForever);
		/* Send CAN message */
		while (osMessageQueueGet(can_outbound_queue, &msg_from_queue,
					 NULL, 0U) == osOK) {
			/* Wait if CAN outbound queue is full */
			while (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0) {
				osDelay(1);
			}

			msg_status = can_send_msg(can1, &msg_from_queue);

			if (msg_status == HAL_ERROR) {
				fault_data.diag = "Failed to send CAN message";
				queue_fault(&fault_data);
			} else if (msg_status == HAL_BUSY) {
				fault_data.diag = "Outbound mailbox full!";
				queue_fault(&fault_data);
			}
		}
	}
}

osThreadId_t can_receive_thread;
const osThreadAttr_t can_receive_attributes = {
	.name = "CanProcessing",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime,
};

void vCanReceive(void *pv_params)
{
	dti_t *mc = (dti_t *)pv_params;
	assert(mc);

	can_msg_t msg;

	for (;;) {
		osThreadFlagsWait(NEW_CAN_MSG_FLAG, osFlagsWaitAny,
				  osWaitForever);
		while (osOK ==
		       osMessageQueueGet(can_inbound_queue, &msg, 0U, 0U)) {
			switch (msg.id) {
			/* Messages Relevant to Motor Controller */
			case DTI_CANID_ERPM:
				dti_record_rpm(mc, msg);
				break;
			case DTI_CANID_TEMPS_FAULT:
				dti_record_temp(mc, msg);
				break;
			case BMS_DCL_MSG:
				handle_dcl_msg();
				break;
			case BUTTON_CANID_IO:
				buttons_update(msg);
				break;
			case DIAL_CANID_IO:
				dial_update(msg);
				break;
			case CONTROL_CANID_FANBATTBOX:
				control_fanbattbox_record(msg);
				break;
			case CONTROL_CANID_PUMP:
				control_pump_record(msg);
			case CONTROL_CANID_RADFAN:
				control_radfan_record(msg);
			default:
				break;
			}
		}
	}
}