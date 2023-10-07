#include <string.h>
#include "queues.h"
#include "cerberus_conf.h"
#include "monitor.h"
#include "task.h"
#include "can.h"
#include "fault.h"
#include "sht30.h"

osThreadId_t temp_monitor_handle;
const osThreadAttr_t temp_monitor_attributes = {
	.name = "TempMonitor",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityBelowNormal3,
};

void vTempMonitor(void *pv_params)
{
	const uint8_t num_samples = 10;
	const uint8_t temp_sensor_sample_delay = 500; /* ms */
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

	if (sht30_init(&temp_sensor, &hi2c1)) {
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