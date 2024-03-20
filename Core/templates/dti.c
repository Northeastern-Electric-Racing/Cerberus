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

{{dispatch}}

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
		{{router}}
	}
}
