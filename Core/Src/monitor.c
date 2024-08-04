#include "monitor.h"
#include "c_utils.h"
#include "can_handler.h"
#include "cerberus_conf.h"
#include "fault.h"
#include "lsm6dso.h"
#include "mpu.h"
#include "pdu.h"
#include "queues.h"
#include "serial_monitor.h"
#include "sht30.h"
#include "state_machine.h"
#include "steeringio.h"
#include "stm32f405xx.h"
#include "task.h"
#include "timer.h"
#include "processing.h"
#include "control.h"
#include "cerb_utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parameters for the pedal monitoring task */
#define MAX_ADC_VAL_12b 4096
// DEBUG: threshold may need adjusting
#define PEDAL_DIFF_THRESH 30
#define PEDAL_FAULT_TIME  500 /* ms */

static bool brake_state = false;

enum { ACCELPIN_2, ACCELPIN_1, BRAKEPIN_1, BRAKEPIN_2 };

osThreadId lv_monitor_handle;
const osThreadAttr_t lv_monitor_attributes = {
	.name = "LVMonitor",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityNormal,
};

void vLVMonitor(void *pv_params)
{
	mpu_t *mpu = (mpu_t *)pv_params;
	fault_data_t fault_data = { .id = LV_MONITOR_FAULT,
				    .severity = DEFCON5 };
	can_msg_t msg = { .id = CANID_LV_MONITOR, .len = 4, .data = { 0 } };

	uint32_t v_int;

	for (;;) {
		read_lv_voltage(mpu, &v_int);

		/* Convert from raw ADC reading to voltage level */

		// scale up then truncate
		// convert to out of 24 volts
		// since 12 bits / 4096
		// Magic number bc idk the resistors on the voltage divider
		float v_dec = v_int * 8.967;

		// get final voltage
		v_int = (uint32_t)(v_dec * 10.0);

		memcpy(msg.data, &v_int, msg.len);
		if (queue_can_msg(msg)) {
			fault_data.diag =
				"Failed to send steering LV monitor CAN message";
			queue_fault(&fault_data);
		}

		osDelay(LV_READ_DELAY);
	}
}

bool get_brake_state()
{
	return brake_state;
}

/**
 * @brief Return the adjusted pedal value based on its offset and maximum value. Clamps negative values to 0.
 * 
 * @param raw the raw pedal value
 * @param offset the offset for the pedal
 * @param max the maximum value of the pedal
 */
uint16_t adjust_pedal_val(uint32_t raw, int32_t offset, int32_t max)
{
	return (int16_t)raw - offset <= 0 ?
		       0 :
		       (uint16_t)(raw - offset) * 100 / (max - offset);
}

/**
 * @brief Callback for pedal fault debouncing.
 * 
 * @param arg The fault message as a char*.
 */
void pedal_fault_cb(void *arg)
{
	fault_data_t fault_data = { .id = ONBOARD_PEDAL_FAULT,
				    .severity = DEFCON1 };
	fault_data.diag = (char *)arg;
	queue_fault(&fault_data);
}

void send_pedal_data(void *arg)
{
	// TODO: validate that this works
	uint32_t *adc_data = (uint32_t *)arg;

	can_msg_t accel_pedals_msg = { .id = CANID_PEDALS_ACCEL_MSG,
				       .len = 8,
				       .data = { 0 } };
	can_msg_t brake_pedals_msg = { .id = CANID_PEDALS_BRAKE_MSG,
				       .len = 8,
				       .data = { 0 } };

	endian_swap(&adc_data[ACCELPIN_1], sizeof(adc_data[ACCELPIN_1]));
	endian_swap(&adc_data[ACCELPIN_2], sizeof(adc_data[ACCELPIN_2]));
	memcpy(accel_pedals_msg.data, &adc_data, accel_pedals_msg.len);
	queue_can_msg(accel_pedals_msg);

	endian_swap(&adc_data[BRAKEPIN_1], sizeof(adc_data[BRAKEPIN_1]));
	endian_swap(&adc_data[BRAKEPIN_2], sizeof(adc_data[BRAKEPIN_2]));
	memcpy(brake_pedals_msg.data, adc_data + 2, brake_pedals_msg.len);
	queue_can_msg(brake_pedals_msg);
}

osThreadId_t pedals_monitor_handle;
const osThreadAttr_t pedals_monitor_attributes = {
	.name = "PedalMonitor",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime,
};

void vPedalsMonitor(void *pv_params)
{
	fault_data_t fault_data = { .id = ONBOARD_PEDAL_FAULT,
				    .severity = DEFCON1 };
	static pedals_t sensor_data;
	uint32_t adc_data[4];
	osTimerId_t send_pedal_data_timer =
		osTimerNew(&send_pedal_data, osTimerOnce, adc_data, NULL);

	/* oc = Open Circuit */
	osTimerId_t oc_fault_timer = osTimerNew(
		&pedal_fault_cb, osTimerOnce,
		"Pedal open circuit fault - max acceleration value", NULL);

	/* sc = Short Circuit */
	osTimerId_t sc_fault_timer = osTimerNew(
		&pedal_fault_cb, osTimerOnce,
		"Pedal short circuit fault - no acceleration value", NULL);

	/* Pedal difference too large fault */
	osTimerId_t diff_fault_timer = osTimerNew(
		&pedal_fault_cb, osTimerOnce,
		"Pedal fault - pedal values are too different", NULL);

	/* Handle ADC Data for two input accelerator value and two input brake value*/
	mpu_t *mpu = (mpu_t *)pv_params;

	for (;;) {
		read_pedals(mpu, adc_data);

		uint32_t accel1_raw = adc_data[ACCELPIN_1];
		uint32_t accel2_raw = adc_data[ACCELPIN_2];

		/* Pedal open circuit fault */
		bool open_circuit = accel1_raw > (MAX_ADC_VAL_12b - 20) ||
				    accel2_raw > (MAX_ADC_VAL_12b - 20);
		debounce(open_circuit, oc_fault_timer, PEDAL_FAULT_TIME);

		/* Pedal short circuit fault */
		bool short_circuit = accel1_raw < 500 || accel2_raw < 500;
		debounce(short_circuit, sc_fault_timer, PEDAL_FAULT_TIME);

		/* Normalize pedal values to be from 0-100 */
		uint16_t accel1_norm = adjust_pedal_val(
			adc_data[ACCELPIN_1], ACCEL1_OFFSET, ACCEL1_MAX_VAL);
		uint16_t accel2_norm = adjust_pedal_val(
			adc_data[ACCELPIN_2], ACCEL2_OFFSET, ACCEL2_MAX_VAL);

		/* Pedal difference fault evaluation */
		bool pedals_too_diff = abs(accel1_norm - accel2_norm) >
				       PEDAL_DIFF_THRESH;
		debounce(pedals_too_diff, diff_fault_timer, PEDAL_FAULT_TIME);

		/* Combine normalized values from both accel pedal sensors */
		uint16_t accel_val = (uint16_t)(accel1_norm + accel2_norm) / 2;
		uint16_t brake_val =
			(adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2;

		brake_state = brake_val > PEDAL_BRAKE_THRESH;
		/* Notify brakelight controls task that there is new brake data */
		osThreadFlagsSet(brakelight_control_thread,
				 BRAKE_STATE_UPDATE_FLAG);

		sensor_data.accelerator_value = accel_val;
		sensor_data.brake_value = brake_val;

		/* Publish to Onboard Pedals Queue */
		osStatus_t check = osMessageQueuePut(pedal_data_queue,
						     &sensor_data, 0U, 0U);
		osThreadFlagsSet(process_pedals_thread, PEDAL_DATA_FLAG);

		if (check != 0) {
			fault_data.diag = "Failed to push pedal data to queue";
			queue_fault(&fault_data);
		}

		/* Send CAN messages with raw pedal readings, we do not care if it fails*/
		if (!osTimerIsRunning(send_pedal_data_timer))
			osTimerStart(send_pedal_data_timer, 100);

		osDelay(PEDALS_SAMPLE_DELAY);
	}
}

osThreadId_t imu_monitor_handle;
const osThreadAttr_t imu_monitor_attributes = {
	.name = "IMUMonitor",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityHigh,
};

void vIMUMonitor(void *pv_params)
{
	const uint8_t num_samples = 10;
	static imu_data_t sensor_data;
	fault_data_t fault_data = { .id = IMU_FAULT, .severity = DEFCON5 };
	can_msg_t imu_accel_msg = { .id = CANID_IMU_ACCEL,
				    .len = 6,
				    .data = { 0 } };
	can_msg_t imu_gyro_msg = { .id = CANID_IMU_GYRO,
				   .len = 6,
				   .data = { 0 } };

	mpu_t *mpu = (mpu_t *)pv_params;

	for (;;) {
		// serial_print("IMU Task\r\n");
		/* Take measurement */
		uint16_t accel_data[3] = { 0 };
		uint16_t gyro_data[3] = { 0 };
		if (read_accel(mpu, accel_data)) {
			fault_data.diag = "Failed to get IMU acceleration";
			queue_fault(&fault_data);
		}

		if (read_gyro(mpu, gyro_data)) {
			fault_data.diag = "Failed to get IMU gyroscope";
			queue_fault(&fault_data);
		}

		/* Run values through LPF of sample size  */
		sensor_data.accel_x =
			(sensor_data.accel_x + accel_data[0]) / num_samples;
		sensor_data.accel_y =
			(sensor_data.accel_y + accel_data[1]) / num_samples;
		sensor_data.accel_z =
			(sensor_data.accel_z + accel_data[2]) / num_samples;
		sensor_data.gyro_x =
			(sensor_data.gyro_x + gyro_data[0]) / num_samples;
		sensor_data.gyro_y =
			(sensor_data.gyro_y + gyro_data[1]) / num_samples;
		sensor_data.gyro_z =
			(sensor_data.gyro_z + gyro_data[2]) / num_samples;

		/* Publish to IMU Queue */
		osMessageQueuePut(imu_queue, &sensor_data, 0U, 0U);

		/* convert to big endian */
		endian_swap(&sensor_data.accel_x, sizeof(sensor_data.accel_x));
		endian_swap(&sensor_data.accel_y, sizeof(sensor_data.accel_y));
		endian_swap(&sensor_data.accel_z, sizeof(sensor_data.accel_z));
		endian_swap(&sensor_data.gyro_x, sizeof(sensor_data.gyro_x));
		endian_swap(&sensor_data.gyro_y, sizeof(sensor_data.gyro_y));
		endian_swap(&sensor_data.gyro_z, sizeof(sensor_data.gyro_z));

		/* Send CAN message */
		memcpy(imu_accel_msg.data, &sensor_data, imu_accel_msg.len);
		// if (queue_can_msg(imu_accel_msg)) {
		//	fault_data.diag = "Failed to send CAN message";
		//	queue_fault(&fault_data);
		// }

		memcpy(imu_gyro_msg.data, &sensor_data, imu_gyro_msg.len);
		if (queue_can_msg(imu_gyro_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		/* Yield to other tasks */
		osDelay(IMU_SAMPLE_DELAY);
	}
}

osThreadId_t tsms_monitor_handle;
const osThreadAttr_t tsms_monitor_attributes = {
	.name = "TsmsMonitor",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityHigh,
};

void vTsmsMonitor(void *pv_params)
{
	fault_data_t fault_data = { .id = FUSE_MONITOR_FAULT,
				    .severity = DEFCON5 };
	pdu_t *pdu = (pdu_t *)pv_params;
	bool tsms_status = false;
	/* ms */
	static const uint8_t TSMS_SENSE_SAMPLE_RATE = 100;

	for (;;) {
		/* If the TSMS reading throws an error, queue TSMS fault */
		if (read_tsms_sense(pdu, &tsms_status)) {
			queue_fault(&fault_data);
		} else {
			osMessageQueuePut(tsms_data_queue, &tsms_status, 0U,
					  0U);
			osThreadFlagsSet(process_tsms_thread_id,
					 TSMS_UPDATE_FLAG);
		}
		osDelay(TSMS_SENSE_SAMPLE_RATE);
	}
}

osThreadId_t fusing_monitor_handle;
const osThreadAttr_t fusing_monitor_attributes = {
	.name = "FusingMonitor",
	.stack_size = 64 * 8,
	.priority = (osPriority_t)osPriorityNormal,
};

void vFusingMonitor(void *pv_params)
{
	fault_data_t fault_data = { .id = FUSE_MONITOR_FAULT,
				    .severity = DEFCON5 };
	can_msg_t fuse_msg = { .id = CANID_FUSE, .len = 2, .data = { 0 } };
	pdu_t *pdu = (pdu_t *)pv_params;
	uint16_t fuse_buf;
	bool fuses[MAX_FUSES] = { 0 };

	struct __attribute__((__packed__)) {
		uint8_t fuse_1;
		uint8_t fuse_2;
	} fuse_data;

	for (;;) {
		fuse_buf = 0;

		if (read_fuses(pdu, fuses)) {
			fault_data.diag = "Failed to read fuses";
			queue_fault(&fault_data);
		}

		for (fuse_t fuse = 0; fuse < MAX_FUSES; fuse++) {
			fuse_buf |=
				fuses[fuse]
				<< fuse; /* Sets the bit at position `fuse` to the state of the fuse */
		}

		fuse_data.fuse_1 = fuse_buf & 0xFF;
		fuse_data.fuse_2 = (fuse_buf >> 8) & 0xFF;

		// reverse the bit order
		fuse_data.fuse_1 = reverse_bits(fuse_data.fuse_1);
		fuse_data.fuse_2 = reverse_bits(fuse_data.fuse_2);

		memcpy(fuse_msg.data, &fuse_data, fuse_msg.len);
		if (queue_can_msg(fuse_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		osDelay(FUSES_SAMPLE_DELAY);
	}
}

osThreadId_t shutdown_monitor_handle;
const osThreadAttr_t shutdown_monitor_attributes = {
	.name = "ShutdownMonitor",
	.stack_size = 64 * 8,
	.priority = (osPriority_t)osPriorityHigh2,
};

void vShutdownMonitor(void *pv_params)
{
	fault_data_t fault_data = { .id = SHUTDOWN_MONITOR_FAULT,
				    .severity = DEFCON5 };
	can_msg_t shutdown_msg = { .id = CANID_SHUTDOWN_LOOP,
				   .len = 2,
				   .data = { 0 } };
	pdu_t *pdu = (pdu_t *)pv_params;
	bool shutdown_loop[MAX_SHUTDOWN_STAGES] = { 0 };
	uint16_t shutdown_buf;

	struct __attribute__((__packed__)) {
		uint8_t shut_1;
		uint8_t shut_2;
	} shutdown_data;

	for (;;) {
		shutdown_buf = 0;

		if (read_shutdown(pdu, shutdown_loop)) {
			fault_data.diag = "Failed to read shutdown buffer";
			queue_fault(&fault_data);
		}

		for (shutdown_stage_t stage = 0; stage < MAX_SHUTDOWN_STAGES;
		     stage++) {
			shutdown_buf |=
				shutdown_loop[stage]
				<< stage; /* Sets the bit at position `stage` to the state of the stage */
		}

		/* seperate each byte */
		shutdown_data.shut_1 = shutdown_buf & 0xFF;
		shutdown_data.shut_2 = (shutdown_buf >> 8) & 0xFF;

		// reverse the bit order
		shutdown_data.shut_2 = reverse_bits(shutdown_data.shut_1);
		shutdown_data.shut_2 = reverse_bits(shutdown_data.shut_2);

		memcpy(shutdown_msg.data, &shutdown_data, shutdown_msg.len);
		if (queue_can_msg(shutdown_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		osDelay(SHUTDOWN_MONITOR_DELAY);
	}
}

osThreadId steeringio_buttons_monitor_handle;
const osThreadAttr_t steeringio_buttons_monitor_attributes = {
	.name = "SteeringIOButtonsMonitor",
	.stack_size = 400,
	.priority = (osPriority_t)osPriorityNormal,
};

void vSteeringIOButtonsMonitor(void *pv_params)
{
	steeringio_t *wheel = (steeringio_t *)pv_params;
	can_msg_t msg = { .id = 0x680, .len = 8, .data = { 0 } };
	fault_data_t fault_data = { .id = BUTTONS_MONITOR_FAULT,
				    .severity = DEFCON5 };

	for (;;) {
		uint8_t button_1 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
		uint8_t button_2 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
		uint8_t button_3 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
		uint8_t button_4 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
		uint8_t button_5 = !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4);
		uint8_t button_6 = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5);
		uint8_t button_7 = !HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
		uint8_t button_8 = !HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);

		uint8_t button_data = (button_1 << 7) | (button_2 << 6) |
				      (button_3 << 5) | (button_4 << 4) |
				      (button_5 << 3) | (button_6 << 2) |
				      (button_7 << 1) | (button_8);

		steeringio_update(wheel, button_data);

		/* Set the first byte to be the first 8 buttons with each bit representing the pin status */
		msg.data[0] = button_data;
		if (queue_can_msg(msg)) {
			fault_data.diag =
				"Failed to send steering buttons can message";
			queue_fault(&fault_data);
		}

		osDelay(25);
	}
}

osThreadId_t temp_monitor_handle;
const osThreadAttr_t temp_monitor_attributes = {
	.name = "TempMonitor",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityHigh1,
};

void vTempMonitor(void *pv_params)
{
	fault_data_t fault_data = { .id = ONBOARD_TEMP_FAULT,
				    .severity = DEFCON5 };
	can_msg_t temp_msg = { .id = CANID_TEMP_SENSOR,
			       .len = 4,
			       .data = { 0 } };

	mpu_t *mpu = (mpu_t *)pv_params;

	for (;;) {
		/* Take measurement */
		uint16_t temp = 0;
		uint16_t humidity = 0;
		if (read_temp_sensor(mpu, &temp, &humidity)) {
			fault_data.diag = "Failed to get temp";
			queue_fault(&fault_data);
		}

		serial_print("MPU Board Temperature:\t%d\r\n", temp);

		temp_msg.data[0] = temp & 0xFF;
		temp_msg.data[1] = (temp >> 8) & 0xFF;
		temp_msg.data[2] = humidity & 0xFF;
		temp_msg.data[3] = (humidity >> 8) & 0xFF;

		/* Send CAN message */
		if (queue_can_msg(temp_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		/* Yield to other tasks */
		osDelay(TEMP_SENS_SAMPLE_DELAY);
	}
}
