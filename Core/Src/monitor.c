#include "monitor.h"
#include "can_handler.h"
#include "cerberus_conf.h"
#include "fault.h"
#include "lsm6dso.h"
#include "mpu.h"
#include "pdu.h"
#include "queues.h"
#include "serial_monitor.h"
#include "sht30.h"
#include "stm32f405xx.h"
#include "task.h"
#include "lsm6dso.h"
#include "timer.h"
#include "serial_monitor.h"
#include "pdu.h"
#include "mpu.h"
#include "timer.h"
#include <stdbool.h>
#include <string.h>

osThreadId_t temp_monitor_handle;
const osThreadAttr_t temp_monitor_attributes = {
	.name		= "TempMonitor",
	.stack_size = 128 * 8,
	.priority	= (osPriority_t)osPriorityNormal1,
};

void vTempMonitor(void* pv_params)
{
	const uint8_t num_samples = 10;
	static onboard_temp_t sensor_data;
	fault_data_t fault_data = { .id = ONBOARD_TEMP_FAULT, .severity = DEFCON4 };
	can_msg_t temp_msg		= { .id = CANID_TEMP_SENSOR, .len = 4, .data = { 0 } };

	//mpu_t *mpu = (mpu_t *)pv_params;

	for (;;) {
		/* Take measurement */
		// serial_print("Temp Sensor Task\r\n");
		uint16_t temp	  = 0;
		uint16_t humidity = 0;
		//if (read_temp_sensor(mpu, &temp, &humidity)) {
		//	fault_data.diag = "Failed to get temp";
		//	queue_fault(&fault_data);
		//}

		/* Run values through LPF of sample size  */
		sensor_data.temperature = (sensor_data.temperature + temp) / num_samples;
		sensor_data.humidity	= (sensor_data.humidity + humidity) / num_samples;

		/* Publish to Onboard Temp Queue */
		osMessageQueuePut(onboard_temp_queue, &sensor_data, 0U, 0U);

		/* Send CAN message */
		memcpy(temp_msg.data, &sensor_data, temp_msg.len);
		if (queue_can_msg(temp_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		/* Yield to other tasks */
		osDelay(TEMP_SENS_SAMPLE_DELAY);
	}
}

osThreadId_t watchdog_monitor_handle;
const osThreadAttr_t watchdog_monitor_attributes = {
	.name		= "WatchdogMonitor",
	.stack_size = 128 * 4,
	.priority	= (osPriority_t)osPriorityNormal,
};

void vWatchdogMonitor(void* pv_params)
{
	mpu_t* mpu = (mpu_t*)pv_params;

	for (;;) {
		/* Pets Watchdog */
		pet_watchdog(mpu);
		/* Yield to other RTOS tasks */
		pet_watchdog(mpu);
		osThreadYield();
	}
}

osThreadId_t pedals_monitor_handle;
const osThreadAttr_t pedals_monitor_attributes = {
	.name		= "PedalMonitor",
	.stack_size = 128 * 12,
	.priority	= (osPriority_t)osPriorityAboveNormal6,
};

void vPedalsMonitor(void* pv_params)
{
	const uint8_t num_samples = 10;
	enum {BRAKEPIN_1, BRAKEPIN_2, ACCELPIN_1, ACCELPIN_2 };
	// nertimer_t diff_timer;
	// nertimer_t sc_timer;
	// nertimer_t oc_timer;
	static pedals_t sensor_data;
	fault_data_t fault_data = { .id = ONBOARD_PEDAL_FAULT, .severity = DEFCON1 };
	//can_msg_t pedal_msg		= { .id = CANID_PEDAL_SENSOR, .len = 4, .data = { 0 } };
	uint16_t adc_data[3];

	/* Handle ADC Data for two input accelerator value and two input brake value*/
	mpu_t *mpu = (mpu_t *)pv_params;

	//uint32_t curr_tick = HAL_GetTick();

	for (;;) {
		//serial_print("Pedals Task: %d\r\n", HAL_GetTick() - curr_tick);
		//curr_tick = HAL_GetTick();
		if (read_adc(mpu, adc_data)) {
			fault_data.diag = "Failed to collect ADC Data!";
			queue_fault(&fault_data);
		}

		/* Evaluate accelerator faults */
		// if (is_timer_expired(&oc_timer))
		// todo queue fault
		//	continue;
		// else if ((adc_data[ACCELPIN_1] == MAX_ADC_VAL_12B || adc_data[ACCELPIN_2] ==
		// MAX_ADC_VAL_12B) && 	!is_timer_active(&oc_timer)) 	start_timer(&oc_timer,
		// PEDAL_FAULT_TIME); else 	cancel_timer(&oc_timer);

		// if (is_timer_expired(&sc_timer))
		// todo queue fault
		//	continue;
		// else if ((adc_data[ACCELPIN_1] == 0 || adc_data[ACCELPIN_2] == 0) &&
		//	!is_timer_active(&sc_timer))
		//	start_timer(&sc_timer, PEDAL_FAULT_TIME);
		// else
		//	cancel_timer(&sc_timer);

		// if (is_timer_expired(&diff_timer))
		// todo queue fault
		//	continue;
		// else if ((adc_data[ACCELPIN_1] - adc_data[ACCELPIN_2] > PEDAL_DIFF_THRESH *
		// MAX_ADC_VAL_12B) && 	!is_timer_active(&diff_timer)) 	start_timer(&diff_timer,
		// PEDAL_FAULT_TIME); else 	cancel_timer(&diff_timer);

		/* Low Pass Filter */
		sensor_data.accelerator_value
			= (sensor_data.accelerator_value + (adc_data[ACCELPIN_1] + adc_data[ACCELPIN_2]) / 2)
			  / num_samples;
		 sensor_data.brake_value
			= (sensor_data.brake_value + (adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2)
			  / num_samples;

		/* Publish to Onboard Pedals Queue */
		osStatus_t check = osMessageQueuePut(pedal_data_queue, &sensor_data, 0U, 0U);
		
		if(check != 0)
		{
			fault_data.diag = "Failed to push pedal data to queue";
			queue_fault(&fault_data);
		}
		osDelay(PEDALS_SAMPLE_DELAY);
	}
}

osThreadId_t imu_monitor_handle;
const osThreadAttr_t imu_monitor_attributes = {
	.name		= "IMUMonitor",
	.stack_size = 128 * 8,
	.priority	= (osPriority_t)osPriorityAboveNormal2,
};

void vIMUMonitor(void* pv_params)
{
	const uint8_t num_samples = 10;
	static imu_data_t sensor_data;
	fault_data_t fault_data = { .id = IMU_FAULT, .severity = DEFCON3 };
	can_msg_t imu_accel_msg = { .id = CANID_IMU, .len = 6, .data = { 0 } };
	can_msg_t imu_gyro_msg	= { .id = CANID_IMU, .len = 6, .data = { 0 } };

	mpu_t *mpu = (mpu_t *)pv_params;

	for (;;) {
		// serial_print("IMU Task\r\n");
		/* Take measurement */
		uint16_t accel_data[3] = { 0 };
		uint16_t gyro_data[3]  = { 0 };
		if (read_accel(mpu,accel_data)) {
			fault_data.diag = "Failed to get IMU acceleration";
			queue_fault(&fault_data);
		}

		if (read_gyro(mpu, gyro_data)) {
			fault_data.diag = "Failed to get IMU gyroscope";
			queue_fault(&fault_data);
		}

		/* Run values through LPF of sample size  */
		sensor_data.accel_x = (sensor_data.accel_x + accel_data[0]) / num_samples;
		sensor_data.accel_y = (sensor_data.accel_y + accel_data[1]) / num_samples;
		sensor_data.accel_z = (sensor_data.accel_z + accel_data[2]) / num_samples;
		sensor_data.gyro_x	= (sensor_data.gyro_x + gyro_data[0]) / num_samples;
		sensor_data.gyro_y	= (sensor_data.gyro_y + gyro_data[1]) / num_samples;
		sensor_data.gyro_z	= (sensor_data.gyro_z + gyro_data[2]) / num_samples;

		/* Publish to IMU Queue */
		osMessageQueuePut(imu_queue, &sensor_data, 0U, 0U);

		/* Send CAN message */
		memcpy(imu_accel_msg.data, &sensor_data, imu_accel_msg.len);
		//if (queue_can_msg(imu_accel_msg)) {
		//	fault_data.diag = "Failed to send CAN message";
		//	queue_fault(&fault_data);
		//}

		memcpy(imu_gyro_msg.data, &sensor_data, imu_gyro_msg.len);
		if (queue_can_msg(imu_gyro_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		/* Yield to other tasks */
		osDelay(IMU_SAMPLE_DELAY);
	}
}

osThreadId_t fusing_monitor_handle;
const osThreadAttr_t fusing_monitor_attributes = {
	.name		= "FusingMonitor",
	.stack_size = 128 * 8,
	.priority	= (osPriority_t)osPriorityNormal5,
};

void vFusingMonitor(void* pv_params)
{
	fault_data_t fault_data = { .id = FUSE_MONITOR_FAULT, .severity = DEFCON3 };
	can_msg_t fuse_msg		= { .id = CANID_FUSE, .len = 8, .data = { 0 } };
	pdu_t* pdu				= (pdu_t*)pv_params;
	bool fuses[MAX_FUSES]	= { 0 };
	uint16_t fuse_buf;

	for (;;) {
		fuse_buf = 0;

		for (fuse_t fuse = 0; fuse < MAX_FUSES; fuse++) {
			if (read_fuse(pdu, fuse, &fuses[fuse])) /* Actually read the fuse */
				fuse_buf |= 1 << MAX_FUSES;			/* Set error bit */

			fuse_buf |= fuses[fuse]
						<< fuse; /* Sets the bit at position `fuse` to the state of the fuse */
		}

		memcpy(fuse_msg.data, &fuse_buf, fuse_msg.len);
		if (queue_can_msg(fuse_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		osDelay(FUSES_SAMPLE_DELAY);
	}
}

osThreadId_t shutdown_monitor_handle;
const osThreadAttr_t shutdown_monitor_attributes = {
	.name		= "ShutdownMonitor",
	.stack_size = 128 * 8,
	.priority	= (osPriority_t)osPriorityNormal3,
};

void vShutdownMonitor(void* pv_params)
{
	fault_data_t fault_data = { .id = SHUTDOWN_MONITOR_FAULT, .severity = DEFCON2 };
	can_msg_t shutdown_msg	= { .id = CANID_SHUTDOWN_LOOP, .len = 8, .data = { 0 } };
	pdu_t* pdu				= (pdu_t*)pv_params;
	bool shutdown_loop[MAX_SHUTDOWN_STAGES] = { 0 };
	uint16_t shutdown_buf;

	for (;;) {
		shutdown_buf = 0;

		for (shutdown_stage_t stage = 0; stage < MAX_SHUTDOWN_STAGES; stage++) {
			if (read_shutdown(
					pdu, stage,
					&shutdown_loop[stage])) /* Actually read the shutdown loop stage state */
				shutdown_buf |= 1 << MAX_SHUTDOWN_STAGES; /* Set error bit */

			shutdown_buf
				|= shutdown_loop[stage]
				   << stage; /* Sets the bit at position `stage` to the state of the stage */
		}

		memcpy(shutdown_msg.data, &shutdown_buf, shutdown_msg.len);
		if (queue_can_msg(shutdown_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		osDelay(SHUTDOWN_MONITOR_DELAY);
	}
}
