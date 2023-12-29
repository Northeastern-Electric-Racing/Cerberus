#include <string.h>
#include <stdbool.h>
#include "monitor.h"
#include "can.h"
#include "cerberus_conf.h"
#include "fault.h"
#include "queues.h"
#include "sht30.h"
#include "stm32f405xx.h"
#include "task.h"
#include "lsm6dso.h"
#include "timer.h"
#include "serial_monitor.h"
#include "pdu.h"

osThreadId_t temp_monitor_handle;
const osThreadAttr_t temp_monitor_attributes = {
	.name		= "TempMonitor",
	.stack_size = 128 * 8,
	.priority	= (osPriority_t)osPriorityNormal1,
};

void vTempMonitor(void* pv_params)
{
	const uint8_t num_samples				= 10;
	const uint16_t temp_sensor_sample_delay = TEMP_SENS_SAMPLE_DELAY;
	const uint8_t can_msg_len				= 4;   /* bytes */
	static onboard_temp_t sensor_data;
	fault_data_t fault_data = { .id = ONBOARD_TEMP_FAULT, .severity = DEFCON4 };
	sht30_t temp_sensor;
	I2C_HandleTypeDef* hi2c1;
	can_msg_t temp_msg
		= { .id = CANID_TEMP_SENSOR, .len = can_msg_len, .data = { 0 } };

	hi2c1				   = (I2C_HandleTypeDef*)pv_params;
	temp_sensor.i2c_handle = hi2c1;

	//if (sht30_init(&temp_sensor)) {
	//	fault_data.diag = "Temp Monitor Init Failed";
	//	queue_fault(&fault_data);
	//}

	for (;;) {
		/* Take measurement */
		//serial_print("Temp Sensor Task\r\n");
		//if (sht30_get_temp_humid(&temp_sensor)) {
		//	fault_data.diag = "Failed to get temp";
		//	queue_fault(&fault_data);
		//}

		/* Run values through LPF of sample size  */
		//sensor_data.temperature = (sensor_data.temperature + temp_sensor.temp) / num_samples;
		//sensor_data.humidity	= (sensor_data.humidity + temp_sensor.humidity) / num_samples;

		/* Publish to Onboard Temp Queue */
		//osMessageQueuePut(onboard_temp_queue, &sensor_data, 0U, 0U);

		/* Send CAN message */
		//memcpy(temp_msg.data, &sensor_data, can_msg_len);
		//if (queue_can_msg(temp_msg)) {
		//	fault_data.diag = "Failed to send CAN message";
		//	queue_fault(&fault_data);
		//}

		/* Yield to other tasks */
		osDelay(temp_sensor_sample_delay);
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
	GPIO_TypeDef* gpio;
	gpio = (GPIO_TypeDef*)pv_params;

	for (;;) {
		/* Pets Watchdog */
		HAL_GPIO_WritePin(gpio, GPIO_PIN_15, GPIO_PIN_SET);
		HAL_GPIO_WritePin(gpio, GPIO_PIN_15, GPIO_PIN_RESET);

		/* Yield to other RTOS tasks */
		osThreadYield();
	}
}

osThreadId_t pedals_monitor_handle;
const osThreadAttr_t pedals_monitor_attributes = {
	.name		= "PedalMonitor",
	.stack_size = 128 * 12,
	.priority	= (osPriority_t)osPriorityHigh,
};

void vPedalsMonitor(void* pv_params)
{
	const uint8_t num_samples = 10;
	enum { ACCELPIN_1, ACCELPIN_2, BRAKEPIN_1, BRAKEPIN_2 };
	const uint16_t delay_time  = PEDALS_SAMPLE_DELAY;
	const uint16_t adc_sample_time = 2; /* ms */
	const uint8_t can_msg_len = 4;	/* bytes */

	nertimer_t diff_timer;
	nertimer_t sc_timer;
	nertimer_t oc_timer;

	static pedals_t sensor_data;
	fault_data_t fault_data = { .id = ONBOARD_PEDAL_FAULT, .severity = DEFCON1 };

	can_msg_t pedal_msg
		= { .id = CANID_PEDAL_SENSOR, .len = can_msg_len, .data = { 0 } };

	/* Handle ADC Data for two input accelerator value and two input brake value*/
	pedal_params_t* params = (pedal_params_t*)pv_params;

	uint16_t adc_data[4];

	uint32_t curr_tick = HAL_GetTick();

	for (;;) {
		//serial_print("Pedals Task: %d\r\n", HAL_GetTick() - curr_tick);
		/*
		 * Get the value from the adc at the brake and accelerator
		 * pin addresses and average them to the sensor data value
		 */
		HAL_ADC_Start(params->accel_adc1);
		HAL_ADC_Start(params->accel_adc2);
		HAL_ADC_Start(params->brake_adc);

		/* Yield to other tasks */
		osDelay(delay_time);

		HAL_StatusTypeDef err = HAL_ADC_PollForConversion(params->accel_adc1, HAL_MAX_DELAY);
		if (!err)
			HAL_ADC_GetValue(params->accel_adc1);

		err = HAL_ADC_PollForConversion(params->accel_adc2, HAL_MAX_DELAY);
		if (!err)
			HAL_ADC_GetValue(params->accel_adc2);
		
		err = HAL_ADC_PollForConversion(params->brake_adc, HAL_MAX_DELAY);
		if (!err)
			HAL_ADC_GetValue(params->brake_adc);

		//err = HAL_ADC_PollForConversion(params->brake_adc, HAL_MAX_DELAY);
		//if (!err)
		  //serial_print("Brake 2: %d\r\n", HAL_ADC_GetValue(params->brake_adc));

		/* Evaluate accelerator faults */
		//if (is_timer_expired(&oc_timer))
			//todo queue fault
		//	continue;
		//else if ((adc_data[ACCELPIN_1] == MAX_ADC_VAL_12B || adc_data[ACCELPIN_2] == MAX_ADC_VAL_12B) &&
		//	!is_timer_active(&oc_timer))
		//	start_timer(&oc_timer, PEDAL_FAULT_TIME);
		//else
		//	cancel_timer(&oc_timer);
		
		//if (is_timer_expired(&sc_timer))
			//todo queue fault
		//	continue;
		//else if ((adc_data[ACCELPIN_1] == 0 || adc_data[ACCELPIN_2] == 0) &&
		//	!is_timer_active(&sc_timer))
		//	start_timer(&sc_timer, PEDAL_FAULT_TIME);
		//else
		//	cancel_timer(&sc_timer);

		//if (is_timer_expired(&diff_timer))
			//todo queue fault
		//	continue;
		//else if ((adc_data[ACCELPIN_1] - adc_data[ACCELPIN_2] > PEDAL_DIFF_THRESH * MAX_ADC_VAL_12B) &&
		//	!is_timer_active(&diff_timer))
		//	start_timer(&diff_timer, PEDAL_FAULT_TIME);
		//else
		//	cancel_timer(&diff_timer);

		//sensor_data.acceleratorValue
		//	= (sensor_data.acceleratorValue + (adc_data[ACCELPIN_1] + adc_data[ACCELPIN_2]) / 2)
		//	  / num_samples;
		//sensor_data.brakeValue
		//	= (sensor_data.brakeValue + (adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2)
		//	  / num_samples;

		/* Publish to Onboard Pedals Queue */
		osMessageQueuePut(pedal_data_queue, &sensor_data, 0U, 0U);

		/* Send CAN message */
		//memcpy(pedal_msg.data, &sensor_data, can_msg_len);
		//if (queue_can_msg(pedal_msg)) {
		//	fault_data.diag = "Failed to send CAN message";
		//	queue_fault(&fault_data);
		//}
  }
}

osThreadId_t imu_monitor_handle;
const osThreadAttr_t imu_monitor_attributes = {
	.name = "IMUMonitor",
	.stack_size = 128 * 8,
	.priority = (osPriority_t) osPriorityAboveNormal2,
};

void vIMUMonitor(void *pv_params)
{
	const uint8_t num_samples = 10;
	const uint16_t imu_sample_delay = IMU_SAMPLE_DELAY;
	const uint8_t accel_msg_len = 6; /* bytes */
	const uint8_t gyro_msg_len = 6; /* bytes */
	static imu_data_t sensor_data;
	fault_data_t fault_data = {
		.id = IMU_FAULT,
		.severity = DEFCON3
	};
	lsm6dso_t imu;
	can_msg_t imu_accel_msg = {
		.id = CANID_IMU,
		.len = accel_msg_len,
		.data = {0}
	};
	can_msg_t imu_gyro_msg = {
		.id = CANID_IMU,
		.len = gyro_msg_len,
		.data = {0}
	};

	I2C_HandleTypeDef *hi2c1 = (I2C_HandleTypeDef *)pv_params;
	imu.i2c_handle = hi2c1;

	/* Initialize IMU */
	//if (lsm6dso_init(&imu, hi2c1)) {
	//	fault_data.diag = "IMU Monitor Init Failed";
	//	queue_fault(&fault_data);
	//}

	for(;;) {
		//serial_print("IMU Task\r\n");
		/* Take measurement */
		//if (lsm6dso_read_accel(&imu)) {
		//	fault_data.diag = "Failed to get IMU acceleration";
		//	queue_fault(&fault_data);
		//}

		//if (lsm6dso_read_gyro(&imu)) {
		//	fault_data.diag = "Failed to get IMU gyroscope";
		//	queue_fault(&fault_data);
		//}

		/* Run values through LPF of sample size  */
		//sensor_data.accel_x = (sensor_data.accel_x + imu.accel_data[0])
		//					  / num_samples;
		//sensor_data.accel_y = (sensor_data.accel_y + imu.accel_data[1])
		//					  / num_samples;
		//sensor_data.accel_z = (sensor_data.accel_z + imu.accel_data[2])
		//					  / num_samples;
		//sensor_data.gyro_x = (sensor_data.gyro_x + imu.gyro_data[0])
		//					 / num_samples;
		//sensor_data.gyro_y = (sensor_data.gyro_y + imu.gyro_data[1])
		//					 / num_samples;
		//sensor_data.gyro_z = (sensor_data.gyro_z + imu.gyro_data[2])
		//					 / num_samples;

		/* Publish to IMU Queue */
		//osMessageQueuePut(imu_queue, &sensor_data, 0U, 0U);

		/* Send CAN message */
		//memcpy(imu_accel_msg.data, &sensor_data, accel_msg_len);
		//if (queue_can_msg(imu_accel_msg)) {
		//	fault_data.diag = "Failed to send CAN message";
		//	queue_fault(&fault_data);
		//}
		
		//memcpy(imu_gyro_msg.data, &sensor_data, gyro_msg_len);
		//if (queue_can_msg(imu_gyro_msg)) {
		//	fault_data.diag = "Failed to send CAN message";
		//	queue_fault(&fault_data);
		//}

		/* Yield to other tasks */
		osDelay(imu_sample_delay);
	}
}

osThreadId_t fusing_monitor_handle;
const osThreadAttr_t fusing_monitor_attributes = {
	.name = "FusingMonitor",
	.stack_size = 128 * 8,
	.priority = (osPriority_t) osPriorityNormal3,
};

void vFusingMonitor(void *pv_params)
{
	const uint8_t fuse_msg_len = 8;

	fault_data_t fault_data = {
		.id = FUSE_MONITOR_FAULT,
		.severity = DEFCON3
	};

	can_msg_t fuse_msg = {
		.id = CANID_FUSE,
		.len = 8,
		.data = {0}
	};

	I2C_HandleTypeDef *hi2c1 = (I2C_HandleTypeDef *)pv_params;

	pdu_t *pdu = init_pdu(hi2c1);

	bool fuses[MAX_FUSES] = { 0 };

	for (;;) {
		for (fuse_t fuse; fuse < MAX_FUSES; fuse++) {
			read_fuse(pdu, fuse, &fuses[fuse]);
		}

		memcpy(fuse_msg.data, &fuses, 8);
		if (queue_can_msg(fuse_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		osDelay(FUSES_SAMPLE_DELAY);
	}
}
