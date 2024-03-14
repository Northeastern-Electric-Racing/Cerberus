/**
 * @file dti.c
 * @author Hamza Iqbal and Nick DePatie
 * @brief Source file for Motor Controller Driver
 * @version 0.1
 * @date 2023-08-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "dti.h"
#include "can.h"
#include "emrax.h"
#include "fault.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "serial_monitor.h"

#define CAN_QUEUE_SIZE 5 /* messages */

static osMutexAttr_t dti_mutex_attributes;
osMessageQueueId_t dti_router_queue;

dti_t* dti_init()
{
	dti_t* mc = malloc(sizeof(dti_t));
	assert(mc);

	/* Create Mutex */
	mc->mutex = osMutexNew(&dti_mutex_attributes);
	assert(mc->mutex);

	// TODO: Set safety parameters for operation (maxes, mins, etc)
	// dti_set_max_ac_brake_current();
	// dti_set_max_ac_current();
	// dti_set_max_dc_brake_current();
	// dti_set_max_dc_current();

	/* Create Queue for CAN signaling */
	dti_router_queue = osMessageQueueNew(CAN_QUEUE_SIZE, sizeof(can_msg_t), NULL);
	assert(dti_router_queue);

	return mc;
}

void dti_set_torque(int16_t torque)
{
	int16_t ac_current = (torque / EMRAX_KT); /* times 10 */
	dti_set_current(ac_current);
}

void dti_set_current(int16_t current)
{
	can_msg_t msg = { .id = 0x0108, .len = 2, .data = { 0 } };
	dti_set_drive_enable(true);
	/* Send CAN message */
	memcpy(&msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_brake_current(int16_t brake_current)
{
	can_msg_t msg = { .id = 0x0208, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(&msg.data, &brake_current, msg.len);
	queue_can_msg(msg);
}

void dti_set_speed(int32_t rpm)
{
	can_msg_t msg = { .id = 0x0308, .len = 4, .data = { 0 } };

	rpm = rpm * EMRAX_NUM_POLE_PAIRS;

	/* Send CAN message */
	memcpy(msg.data, &rpm, msg.len);
	queue_can_msg(msg);
}

void dti_set_position(int16_t angle)
{
	can_msg_t msg = { .id = 0x0408, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &angle, msg.len);
	queue_can_msg(msg);
}

void dti_set_relative_current(int16_t relative_current)
{
	can_msg_t msg = { .id = 0x0508, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &relative_current, msg.len);
	queue_can_msg(msg);
}

void dti_set_relative_brake_current(int16_t relative_brake_current)
{
	can_msg_t msg = { .id = 0x0608, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &relative_brake_current, msg.len);
	queue_can_msg(msg);
}

void dti_set_digital_output(uint8_t output, bool value)
{
	can_msg_t msg = { .id = 0x0708, .len = 1, .data = { 0 } };

	uint8_t ctrl = value >> output;

	/* Send CAN message */
	memcpy(msg.data, &ctrl, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_ac_current(int16_t current)
{
	can_msg_t msg = { .id = 0x0808, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_ac_brake_current(int16_t current)
{
	can_msg_t msg = { .id = 0x0908, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_dc_current(int16_t current)
{
	can_msg_t msg = { .id = 0x0A08, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_dc_brake_current(int16_t current)
{
	can_msg_t msg = { .id = 0x0B08, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_drive_enable(bool drive_enable)
{
	can_msg_t msg = { .id = 0x0C08, .len = 1, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &drive_enable, msg.len);
	queue_can_msg(msg);
}

/* Inbound Task-specific Info */
osThreadId_t dti_router_handle;
const osThreadAttr_t dti_router_attributes
	= { .name = "DTIRouter", .stack_size = 128 * 8, .priority = (osPriority_t)osPriorityNormal3 };

void vDTIRouter(void* pv_params)
{
	can_msg_t message;
	osStatus_t status;
	// fault_data_t fault_data = { .id = DTI_ROUTING_FAULT, .severity = DEFCON2 };

	// dti_t *mc = (dti_t *)pv_params;

	for (;;) {
		/* Wait until new CAN message comes into queue */
		status = osMessageQueueGet(dti_router_queue, &message, NULL, osWaitForever);
		if (status == osOK) 
		{
			switch (message.id) 
			{
				case DTI_CANID_ERPM:
					break;
				case DTI_CANID_CURRENTS:
					break;
				case DTI_CANID_TEMPS_FAULT:
					break;
				case DTI_CANID_ID_IQ:
					break;
				case DTI_CANID_SIGNALS:
					break;
				default:
					break;
			}
		}
	}
}

