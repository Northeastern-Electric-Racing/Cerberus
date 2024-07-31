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
#include "c_utils.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "bms.h"
#include "serial_monitor.h"

#define CAN_QUEUE_SIZE 5 /* messages */
#define SAMPLES	       20
static osMutexAttr_t dti_mutex_attributes;

dti_t *dti_init()
{
	dti_t *mc = malloc(sizeof(dti_t));
	assert(mc);

	/* Create Mutex */
	mc->mutex = osMutexNew(&dti_mutex_attributes);
	assert(mc->mutex);

	return mc;
}

void dti_set_torque(int16_t torque)
{
	/* We can't change motor speed super fast else we blow diff, therefore low pass filter */
	// Static variables for the buffer and index
	static float buffer[SAMPLES] = { 0 };
	static int index = 0;

	// Add the new value to the buffer
	buffer[index] = torque;

	// Increment the index, wrapping around if necessary
	index = (index + 1) % SAMPLES;

	// Calculate the average of the buffer
	float sum = 0.0;
	for (int i = 0; i < SAMPLES; ++i) {
		sum += buffer[i];
	}
	float average = sum / SAMPLES;

	if (torque == 0) {
		average = 0;
	}

	int16_t ac_current = (((float)average / EMRAX_KT) * 10); /* times 10 */

	// serial_print("Commanded Current: %d \r\n", ac_current);

	dti_set_current(ac_current);
}

void dti_set_regen(uint16_t current_target)
{
	/* Simple moving average to smooth change in braking target */

	// Static variables for the buffer and index
	static int16_t buffer[SAMPLES] = { 0 };
	static int index = 0;

	// Add the new value to the buffer
	buffer[index] = current_target;

	// Increment the index, wrapping around if necessary
	index = (index + 1) % SAMPLES;

	// Calculate the average of the buffer
	float sum = 0.0;
	for (int i = 0; i < SAMPLES; ++i) {
		sum += buffer[i];
	}
	float average = sum / SAMPLES;

	if (current_target == 0) {
		average = 0;
	}

	dti_send_brake_current(average);
}

void dti_set_current(int16_t current)
{
	can_msg_t msg = { .id = 0x036, .len = 2, .data = { 0 } };
	dti_set_drive_enable(true);
	/* Send CAN message in big endian format */

	//endian_swap(&current, sizeof(current));
	//memcpy(msg.data, &current, msg.len);
	int8_t msb = (int8_t)((current >> 8) & 0xFF);
	int8_t lsb = (uint8_t)(current & 0xFF);

	msg.data[0] = msb;
	msg.data[1] = lsb;

	queue_can_msg(msg);
}

void dti_send_brake_current(float brake_current)
{
	can_msg_t msg = { .id = 0x056,
			  .len = 8,
			  .data = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };
	/* Motor controller expects int value * 10 */
	int16_t adjusted_brake_current = brake_current * 10;
	/* convert to big endian */
	endian_swap(&adjusted_brake_current, sizeof(adjusted_brake_current));
	/* Send CAN message */
	memcpy(&msg.data, &brake_current, 2);
	queue_can_msg(msg);
}

void dti_set_speed(int32_t rpm)
{
	can_msg_t msg = { .id = 0x076, .len = 4, .data = { 0 } };

	rpm = rpm * EMRAX_NUM_POLE_PAIRS;

	/* convert to big endian */
	endian_swap(&rpm, sizeof(rpm));

	/* Send CAN message */
	memcpy(msg.data, &rpm, msg.len);
	queue_can_msg(msg);
}

void dti_set_position(int16_t angle)
{
	can_msg_t msg = { .id = 0x096, .len = 2, .data = { 0 } };

	/* convert to big endian */
	endian_swap(&angle, sizeof(angle));

	/* Send CAN message */
	memcpy(msg.data, &angle, msg.len);
	queue_can_msg(msg);
}

void dti_set_relative_current(int16_t relative_current)
{
	can_msg_t msg = { .id = 0x0B6, .len = 2, .data = { 0 } };

	/* convert to big endian */
	endian_swap(&relative_current, sizeof(relative_current));

	/* Send CAN message */
	memcpy(msg.data, &relative_current, msg.len);
	queue_can_msg(msg);
}

void dti_set_relative_brake_current(int16_t relative_brake_current)
{
	can_msg_t msg = { .id = 0x0D6, .len = 2, .data = { 0 } };

	/* convert to big endian */
	endian_swap(&relative_brake_current, sizeof(relative_brake_current));

	/* Send CAN message */
	memcpy(msg.data, &relative_brake_current, msg.len);
	queue_can_msg(msg);
}

void dti_set_digital_output(uint8_t output, bool value)
{
	can_msg_t msg = { .id = 0x0F6, .len = 1, .data = { 0 } };

	uint8_t ctrl = value >> output;

	/* Send CAN message */
	memcpy(msg.data, &ctrl, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_ac_current(int16_t current)
{
	can_msg_t msg = { .id = 0x116, .len = 2, .data = { 0 } };

	/* convert to big endian */
	endian_swap(&current, sizeof(current));

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_ac_brake_current(int16_t current)
{
	can_msg_t msg = { .id = 0x136, .len = 2, .data = { 0 } };

	/* convert to big endian */
	endian_swap(&current, sizeof(current));

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_dc_current(int16_t current)
{
	can_msg_t msg = { .id = 0x156, .len = 2, .data = { 0 } };

	/* convert to big endian */
	endian_swap(&current, sizeof(current));

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_dc_brake_current(int16_t current)
{
	can_msg_t msg = { .id = 0x176, .len = 2, .data = { 0 } };

	/* convert to big endian */
	endian_swap(&current, sizeof(current));

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_drive_enable(bool drive_enable)
{
	can_msg_t msg = { .id = 0x196, .len = 1, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &drive_enable, msg.len);
	queue_can_msg(msg);
}

int32_t dti_get_rpm(dti_t *mc)
{
	int32_t rpm;
	osMutexAcquire(*mc->mutex, osWaitForever);
	rpm = mc->rpm;
	//printf("Rpm %ld",rpm);
	osMutexRelease(*mc->mutex);

	return rpm;
}

/* Inbound Task-specific Info */
osThreadId_t dti_router_handle;
const osThreadAttr_t dti_router_attributes = {
	.name = "DTIRouter",
	.stack_size = 64 * 8,
	.priority = (osPriority_t)osPriorityHigh
};

static void dti_record_rpm(dti_t *mc, can_msg_t msg)
{
	/* ERPM is first four bytes of can message in big endian format */
	int32_t erpm = (msg.data[0] << 24) + (msg.data[1] << 16) +
		       (msg.data[2] << 8) + (msg.data[3]);

	int32_t rpm = erpm / POLE_PAIRS;

	//printf("\n\nRPM Rec: %ld\n\n", rpm);

	osMutexAcquire(*mc->mutex, osWaitForever);
	mc->rpm = rpm;
	osMutexRelease(*mc->mutex);
}

void vDTIRouter(void *pv_params)
{
	can_msg_t message;
	osStatus_t status;
	// fault_data_t fault_data = { .id = DTI_ROUTING_FAULT, .severity = DEFCON2 };

	dti_t *mc = (dti_t *)pv_params;

	for (;;) {
		/* Wait until new CAN message comes into queue */
		status = osMessageQueueGet(dti_router_queue, &message, NULL,
					   osWaitForever);
		if (status == osOK) {
			switch (message.id) {
			case DTI_CANID_ERPM:
				dti_record_rpm(mc, message);
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
