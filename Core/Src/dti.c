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
	int16_t ac_current = ((float)torque / EMRAX_KT) * 10; /* times 10 */
	dti_set_current(ac_current);
}

void dti_set_current(int16_t current)
{
	can_msg_t msg = { .id = 0x036, .len = 2, .data = { 0 } };
	dti_set_drive_enable(true);
	/* Send CAN message */
	memcpy(&msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_brake_current(int16_t brake_current)
{
	can_msg_t msg = { .id = 0x056, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(&msg.data, &brake_current, msg.len);
	queue_can_msg(msg);
}

void dti_set_speed(int32_t rpm)
{
	can_msg_t msg = { .id = 0x076, .len = 4, .data = { 0 } };

	rpm = rpm * EMRAX_NUM_POLE_PAIRS;

	/* Send CAN message */
	memcpy(msg.data, &rpm, msg.len);
	queue_can_msg(msg);
}

void dti_set_position(int16_t angle)
{
	can_msg_t msg = { .id = 0x096, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &angle, msg.len);
	queue_can_msg(msg);
}

void dti_set_relative_current(int16_t relative_current)
{
	can_msg_t msg = { .id = 0x0B6, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &relative_current, msg.len);
	queue_can_msg(msg);
}

void dti_set_relative_brake_current(int16_t relative_brake_current)
{
	can_msg_t msg = { .id = 0x0D6, .len = 2, .data = { 0 } };

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

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_ac_brake_current(int16_t current)
{
	can_msg_t msg = { .id = 0x136, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_dc_current(int16_t current)
{
	can_msg_t msg = { .id = 0x156, .len = 2, .data = { 0 } };

	/* Send CAN message */
	memcpy(msg.data, &current, msg.len);
	queue_can_msg(msg);
}

void dti_set_max_dc_brake_current(int16_t current)
{
	can_msg_t msg = { .id = 0x176, .len = 2, .data = { 0 } };

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

void handle_ERPM(const can_msg_t *msg, dti_t *dti)
{
	fault_data_t fault_data = {
		.id		  = DTI_LIMITS_FAULT,
		.severity = DEFCON2,
	};
	dti->rpm = ((msg->data[0] << 24) | (msg->data[1] << 16) | (msg->data[2] << 8) | (msg->data[3])) / 10;
	dti->duty_cycle = (msg->data[4] << 8) | msg->data[5];
	dti->input_voltage = (msg->data[6] << 8) | msg->data[7];

	if(dti->rpm > MAX_RPM)
	{
		fault_data.diag = "DTI: RPM Limit Exceeded";
		queue_fault(&fault_data);
	}
	if(dti->duty_cycle > MAX_DUTY)
	{
		fault_data.diag = "DTI: Duty Cycle too High";
		queue_fault(&fault_data);
	}
	if(dti->input_voltage < MIN_INPUT_VOLTAGE)
	{
		fault_data.diag = "DTI: Input Voltage too Low";
		queue_fault(&fault_data);
	}
	if(dti->input_voltage > MAX_INPUT_VOLTAGE)
	{
		fault_data.diag = "DTI: Input Voltage too High";
		queue_fault(&fault_data);
	}
}

void handle_currents(const can_msg_t *msg, dti_t *dti)
{
	fault_data_t fault_data = {
		.id		  = DTI_LIMITS_FAULT,
		.severity = DEFCON2,
	};
	dti->ac_current = (msg->data[0] << 8) | msg->data[1];
	dti->dc_current = (msg->data[2] << 8) | msg->data[3];
	
	if(dti->ac_current > MAX_AC_CURRENT)
	{
		fault_data.diag = "DTI: AC Current too High";
		queue_fault(&fault_data);
	}
	if(dti->ac_current < MIN_AC_CURRENT)
	{
		fault_data.diag = "DTI: AC Current too Low";
		queue_fault(&fault_data);
	}
	if(dti->dc_current > MAX_DC_CURRENT)
	{
		fault_data.diag = "DTI: DC Current too High";
		queue_fault(&fault_data);
	}
	if(dti->dc_current < MIN_DC_CURRENT)
	{
		fault_data.diag = "DTI: DC Current too Low";
		queue_fault(&fault_data);
	}
}

void handle_temps_faults(const can_msg_t *msg, dti_t *dti)
{
	fault_data_t fault_data = {
		.id		  = DTI_LIMITS_FAULT,
		.severity = DEFCON2,
	};
	fault_data_t crit_fault_data = {
		.id 	  = DTI_CRITCAL_FAULT,
		.severity = DEFCON1,
	};

	dti->contr_temp = (msg->data[0] << 8) | msg->data[1];
	dti->motor_temp = (msg->data[2] << 8) | msg->data[3];
	dti->fault_code = msg->data[4];

	switch(dti->fault_code)
	{
		case DTI_NO_FAULTS:
			break;
		case DTI_OVERVOLTAGE:
			crit_fault_data.diag = "The input voltage is higher than the set maximum";
			queue_fault(&crit_fault_data);
			break;
		case DTI_UNDERVOLTAGE:
			crit_fault_data.diag = "The input voltage is lower than the set minimum";
			queue_fault(&crit_fault_data);
			break;
		case DTI_DRV:
			crit_fault_data.diag = "Transistor or transistor drive error";
			queue_fault(&crit_fault_data);
			break;
		case DTI_ABS_OVERCURRENT:
			crit_fault_data.diag = "The AC current is higher than the set absolute maximum current";
			queue_fault(&crit_fault_data);
			break;
		case DTI_CTLR_OVERTEMP:
			crit_fault_data.diag = "The controller temperature is higher than the set maximum";
			queue_fault(&crit_fault_data);
			break;
        case DTI_MOTOR_OVERTEMP:
			crit_fault_data.diag = "The motor temperature is higher than the set maximum";
			queue_fault(&crit_fault_data);
			break;
		case DTI_SENSOR_WIRE_FAULT:
			fault_data.diag = "Something went wrong with the sensor differential signal";
			queue_fault(&fault_data);
			break;
		case DTI_SENSOR_GEN_FAULT:
			fault_data.diag = "An error occurred while processing the sensor signals";
			queue_fault(&fault_data);
			break;
		case DTI_CAN_CMD_ERR:
			fault_data.diag = "CAN message received contains parameter out of boundaries";
			queue_fault(&fault_data);
			break;
		case DTI_ANLG_IN_ERR:
			fault_data.diag = "Redundant output out of range";
			queue_fault(&fault_data);
			break;
		default:
			break;
	}

	if(dti->contr_temp > CTRL_TEMP_LIMIT)
	{
		fault_data.diag = "Controller temp too high";
		queue_fault(&fault_data);
	}
	if(dti->motor_temp > MOTOR_TEMP_LIMIT)
	{
		fault_data.diag = "Motor temp too high";
		queue_fault(&fault_data);
	}

}

// Dont know what these values are for but they might be useful in future.
void handle_id_iq(const can_msg_t *msg, dti_t *dti)
{
	fault_data_t fault_data = {
		.id		  = DTI_LIMITS_FAULT,
		.severity = DEFCON2,
	};

	dti->foc_id = (msg->data[0] << 24) |  (msg->data[1] << 16) |  (msg->data[2] << 8) | msg->data[3];
	dti->foc_iq = (msg->data[4] << 24) |  (msg->data[5] << 16) |  (msg->data[6] << 8) | msg->data[7];

	if(dti->foc_id > MAX_FOC_ID)
	{
		fault_data.diag = "FOC Id too large";
		queue_fault(&fault_data);
	}
	if(dti->foc_iq > MAX_FOC_IQ)
	{
		fault_data.diag = "FOC Iq too large";
		queue_fault(&fault_data);
	}
}

void handle_signals(const can_msg_t *msg, dti_t *dti)
{
	dti->throttle_signal = msg->data[0];
	dti->brake_signal = msg->data[1];
	dti->signals.digital_in_1 = (msg->data[2] >> 7) & 0x01;
	dti->signals.digital_in_2 = (msg->data[2] >> 6) & 0x01;
	dti->signals.digital_in_3 = (msg->data[2] >> 5) & 0x01;
	dti->signals.digital_in_4 = (msg->data[2] >> 4) & 0x01;
	dti->signals.digital_out_1 = (msg->data[2] >> 3) & 0x01;
	dti->signals.digital_out_2 = (msg->data[2] >> 2) & 0x01;
	dti->signals.digital_out_3 = (msg->data[2] >> 1) & 0x01;
	dti->signals.digital_out_4 = (msg->data[2]) & 0x01;
	dti->drive_enable = msg->data[3];
	dti->signals.cap_temp_limit = (msg->data[4] >> 7) & 0x01;
	dti->signals.dc_current_limit = (msg->data[4] >> 6) & 0x01;
	dti->signals.drive_enable_limit = (msg->data[4] >> 5) & 0x01;
	dti->signals.igbt_acc_temp_limit = (msg->data[4] >> 4) & 0x01;
	dti->signals.igbt_temp_limit = (msg->data[4] >> 3) & 0x01;
	dti->signals.input_voltage_limit = (msg->data[4] >> 2) & 0x01;
	dti->signals.motor_acc_temp_limit = (msg->data[4] >> 1) & 0x01;
	dti->signals.motor_temp_limit = msg->data[4] & 0x01;
	dti->signals.rpm_min_limit = (msg->data[5] >> 7) & 0x01;
	dti->signals.rpm_max_limit = (msg->data[5] >> 6) &  0x01;
	dti->signals.power_limit = (msg->data[5] >> 5) & 0x01;
	dti->signals.can_map_version = msg->data[6] & 0x01;
}

/* Inbound Task-specific Info */
osThreadId_t dti_router_handle;
const osThreadAttr_t dti_router_attributes
	= { .name = "DTIRouter", .stack_size = 128 * 8, .priority = (osPriority_t)osPriorityNormal3 };

void vDTIRouter(void* pv_params)
{
	can_msg_t message;
	osStatus_t status;
	fault_data_t fault_data = { .id = DTI_ROUTING_FAULT, .severity = DEFCON2 };

	dti_t *mc = (dti_t *)pv_params;

	for (;;) {
		/* Wait until new CAN message comes into queue */
		status = osMessageQueueGet(dti_router_queue, &message, NULL, osWaitForever);
		if (status == osOK) 
		{
			switch (message.id) 
			{
				case DTI_CANID_ERPM:
					handle_ERPM(&message, mc);
					break;
				case DTI_CANID_CURRENTS:
					handle_currents(&message, mc);
					break;
				case DTI_CANID_TEMPS_FAULT:
					handle_temps_faults(&message, mc);
					break;
				case DTI_CANID_ID_IQ:
					handle_id_iq(&message, mc);
					break;
				case DTI_CANID_SIGNALS:
					handle_signals(&message, mc);
					break;
				default:
					break;
			}
		}
		else
		{
			fault_data.diag = "Failed to read from DTI incoming message queue \r \n";
			queue_fault(&fault_data);
		}
	}
}

