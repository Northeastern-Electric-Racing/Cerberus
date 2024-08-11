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
#include "pedals.h"
#include "cerb_utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TSMS_DEBOUNCE_PERIOD 500 /* ms */

static bool tsms = false;
osMutexId_t tsms_mutex;

/**
 * @brief Read the open cell voltage of the LV batteries and send a CAN message with the result.
 * 
 * @param arg Pointer to mpu_t.
 */
void read_lv_sense(void *arg)
{
	mpu_t *mpu = (mpu_t *)arg;
	fault_data_t fault_data = { .id = LV_MONITOR_FAULT,
				    .severity = DEFCON5 };
	can_msg_t msg = { .id = CANID_LV_MONITOR, .len = 4, .data = { 0 } };

	uint32_t v_int;

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
}

/**
 * @brief Read data from the fuse monitor GPIO expander on the PDU.
 * 
 * @param pdu Pointer to pdu_t.
 */
void read_fuse_data(void *arg)
{
	pdu_t *pdu = (pdu_t *)arg;
	fault_data_t fault_data = { .id = FUSE_MONITOR_FAULT,
				    .severity = DEFCON5 };
	can_msg_t fuse_msg = { .id = CANID_FUSE, .len = 2, .data = { 0 } };
	uint16_t fuse_buf;
	bool fuses[MAX_FUSES] = { 0 };

	struct __attribute__((__packed__)) {
		uint8_t fuse_1;
		uint8_t fuse_2;
	} fuse_data;

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
}

osThreadId_t non_functional_data_thead;
const osThreadAttr_t non_functional_data_attributes;

void vNonFunctionalDataCollection(void *pv_params)
{
	non_func_data_args_t *args = (non_func_data_args_t *)pv_params;
	mpu_t *mpu = args->mpu;
	pdu_t *pdu = args->pdu;
	free(args);

	for (;;) {
		read_lv_sense(mpu);
		read_fuse_data(pdu);

		/* delay for 1000 ms (1k ticks at 1000 Hz tickrate) */
		osDelay(1000);
	}
}

bool get_tsms()
{
	bool temp;
	osMutexAcquire(tsms_mutex, osWaitForever);
	temp = tsms;
	osMutexRelease(tsms_mutex);
	return temp;
}

void tsms_debounce_cb(void *arg)
{
	/* Set TSMS state to new debounced value */
	osMutexAcquire(tsms_mutex, osWaitForever);
	tsms = *((bool *)arg);
	osMutexRelease(tsms_mutex);
	/* Tell NERO allaboutit */
	send_nero_msg();
}

/**
 * @brief Read the TSMS signal and debounce it.
 * 
 * @param pdu Pointer to struct representing the PDU.
 */
void read_tsms(pdu_t *pdu)
{
	static nertimer_t timer;
	fault_data_t fault_data = { .id = FUSE_MONITOR_FAULT,
				    .severity = DEFCON5 };
	bool tsms_reading;

	/* If the TSMS reading throws an error, queue TSMS fault */
	if (read_tsms_sense(pdu, &tsms_reading)) {
		queue_fault(&fault_data);
	}

	/* Debounce tsms reading */
	if (tsms_reading)
		debounce(tsms_reading, &timer, TSMS_DEBOUNCE_PERIOD,
			 &tsms_debounce_cb, &tsms_reading);
	else
		/* Since debounce only debounces logic high signals, the reading must be inverted if it is low. Think of this as debouncing a "TSMS off is active" debounce. */
		debounce(!tsms_reading, &timer, TSMS_DEBOUNCE_PERIOD,
			 &tsms_debounce_cb, &tsms_reading);

	if (get_active() && get_tsms() == false) {
		set_home_mode();
	}
}

void steeringio_monitor(steeringio_t *wheel)
{
	can_msg_t msg = { .id = 0x680, .len = 8, .data = { 0 } };
	fault_data_t fault_data = { .id = BUTTONS_MONITOR_FAULT,
				    .severity = DEFCON5 };

	uint8_t button_1 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
	uint8_t button_2 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
	uint8_t button_3 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
	uint8_t button_4 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
	uint8_t button_5 = !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4);
	uint8_t button_6 = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5);
	uint8_t button_7 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
	uint8_t button_8 = !HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);

	uint8_t button_data = (button_1 << 7) | (button_2 << 6) |
			      (button_3 << 5) | (button_4 << 4) |
			      (button_5 << 3) | (button_6 << 2) |
			      (button_7 << 1) | (button_8);

	steeringio_update(wheel, button_data);

	/* Set the first byte to be the first 8 buttons with each bit representing the pin status */
	msg.data[0] = button_data;
	if (queue_can_msg(msg)) {
		fault_data.diag = "Failed to send steering buttons can message";
		queue_fault(&fault_data);
	}
}

osThreadId_t data_collection_thread;
const osThreadAttr_t data_collection_attributes = {
	.name = "DataCollection",
	.stack_size = 32 * 32,
	.priority = (osPriority_t)osPriorityBelowNormal,
};

void vDataCollection(void *pv_params)
{
	data_collection_args_t *args = (data_collection_args_t *)pv_params;
	pdu_t *pdu = args->pdu;
	steeringio_t *wheel = args->wheel;
	free(args);
	static const uint8_t delay = 20;

	tsms_mutex = osMutexNew(NULL);

	for (;;) {
		read_tsms(pdu);
		osDelay(delay);

		steeringio_monitor(wheel);
		osDelay(delay);
	}
}

/* Unused -----------------------------------------------*/

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