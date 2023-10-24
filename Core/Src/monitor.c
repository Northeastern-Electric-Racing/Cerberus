#include <string.h>
#include "queues.h"
#include "cerberus_conf.h"
#include "monitor.h"
#include "task.h"
#include "can.h"
#include "fault.h"
#include "sht30.h"
#include "stm32f405xx.h"

osThreadId_t temp_monitor_handle;
const osThreadAttr_t temp_monitor_attributes = {
	.name = "TempMonitor",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityBelowNormal3,
};

void vTempMonitor(void *pv_params)
{
	const uint8_t num_samples = 10;
	const uint16_t temp_sensor_sample_delay = 500; /* ms */
	const uint8_t can_msg_len = 4; /* bytes */
	static onboard_temp_t sensor_data;
	fault_data_t fault_data = {
		.id = ONBOARD_TEMP_FAULT,
		.severity = DEFCON4
	};
	sht30_t temp_sensor;
	I2C_HandleTypeDef *hi2c1;
	can_msg_t temp_msg = {
		.id = CANID_TEMP_SENSOR,
		.len = can_msg_len,
		.line = CAN_LINE_1,
		.data = {0}
	};

	hi2c1 = (I2C_HandleTypeDef *)pv_params;
	temp_sensor.i2c_handle = hi2c1;

	if (sht30_init(&temp_sensor)) {
		fault_data.diag = "Init Failed";
		osMessageQueuePut(fault_handle_queue, &fault_data , 0U, 0U);
	}

	for(;;) {
		/* Take measurement */
		if (sht30_get_temp_humid(&temp_sensor)) {
			fault_data.diag = "Failed to get temp";
			osMessageQueuePut(fault_handle_queue, &fault_data , 0U, 0U);
		}

		/* Run values through LPF of sample size  */
		sensor_data.temperature = (sensor_data.temperature + temp_sensor.temp)
								  / num_samples;
		sensor_data.humidity = (sensor_data.humidity + temp_sensor.humidity)
							   / num_samples;

		/* Publish to Onboard Temp Queue */
		osMessageQueuePut(onboard_temp_queue, &sensor_data, 0U, 0U);

		/* Send CAN message */
		memcpy(temp_msg.data, &sensor_data, can_msg_len);
		if (can_send_message(temp_msg)) {
			fault_data.diag = "Failed to send CAN message";
			osMessageQueuePut(fault_handle_queue, &fault_data , 0U, 0U);
		}

		/* Yield to other tasks */
		osDelayUntil(temp_sensor_sample_delay);
	}
}

osThreadId_t watchdog_monitor_handle;
const osThreadAttr_t watchdog_monitor_attributes = {
	.name = "WatchdogMonitor",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityLow,
};

void vWatchdogMonitor(void *pv_params)
{
	GPIO_TypeDef *gpio;
	gpio = (GPIO_TypeDef *)pv_params;

	for(;;) {
		/* Pets Watchdog */
		HAL_GPIO_WritePin(gpio, GPIO_PIN_15, GPIO_PIN_SET);

		/* Delay for 5ms */
		osDelay(5);

		/* Set Low Again */
		HAL_GPIO_WritePin(gpio, GPIO_PIN_15, GPIO_PIN_RESET);

		/* Yield to other RTOS tasks */
		osThreadYield();
	}
}
osThreadId_t pedals_monitor_handle;
const osThreadAttr_t pedals_monitor_attributes = {
	.name = "PedalMonitor",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityHigh,
};

void vPedalsMonitor(void *pv_params) {
	const uint8_t num_samples = 10;
	const accelerator1PinAddr = 1;
	//TODO: Get actual pin addresses
	const accelerator2PinAddr = 2;
	const brake1PinAddr = 3;
	const brake2PinAddr = 4;
	//TODO: Get actual delay time
	const delayTime = 500; /* ms */	
	const uint8_t can_msg_len = 4; /* bytes */

	static onboard_pedals_t sensor_data;
	fault_data_t fault_data = {
		.id = ONBOARD_PEDAL_FAULT,
		.severity = DEFCON1
	};

	
	can_msg_t pedal_msg = {
		.id = CANID_PEDAL_SENSOR,
		.len = can_msg_len,
		.line = CAN_LINE_1,
		.data = {0}
	};

	/* Handle ADC Data for two input accelerator value and two input brake value*/
	ADC_HandleTypeDef *hadc1 = (ADC_HandleTypeDef *)pv_params;

	/* STM has a 12 bit resolution so we can mark each value as uint16 */
	uint16_t accel_adc_data[2];
	uint16_t brake_adc_data[2];


	for (;;) {
		/* Get the value from the adc at the brake and accelerator pin addresses and average them to the sensor data value */
		accel_adc_data[0] = HAL_ADC_GetValue(hadc1, accelerator1PinAddr);
		accel_adc_data[1] = HAL_ADC_GetValue(hadc1, accelerator2PinAddr);
		brake_adc_data[0] = HAL_ADC_GetValue(hadc1, brake1PinAddr);
		brake_adc_data[1] = HAL_ADC_GetValue(hadc1, brake2PinAddr);

		sensor_data.acceleratorValue = (sensor_data.acceleratorValue + (accel_adc_data[0] + accel_adc_data[1]) / 2) / num_samples;
		sensor_data.brakeValue = (sensor_data.brakeValue + (brake_adc_data[0] + brake_adc_data[1]) / 2) / num_samples;

		//TODO: 

		/* Publish to Onboard Pedals Queue */
		osMessageQueuePut(onboard_pedals_queue, &sensor_data, 0U, 0U);

		/* Send CAN message */
		memcpy(pedal_msg.data, &sensor_data, can_msg_len);
		if (can_send_message(pedal_msg)) {
			fault_data.diag = "Failed to send CAN message";
			osMessageQueuePut(fault_handle_queue, &fault_data , 0U, 0U);
		}

		/* Yield to other tasks */
		osDelayUntil(delayTime);
	}
}
