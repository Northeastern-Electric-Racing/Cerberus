#include "monitor.h"
#include "can_handler.h"
#include "cerberus_conf.h"
#include "fault.h"
#include "mpu.h"
#include "pdu.h"
#include "queues.h"
#include "serial_monitor.h"
#include "sht30.h"
#include "c_utils.h"
#include "stm32f405xx.h"
#include "task.h"
#include "lsm6dso.h"
#include "timer.h"
#include "serial_monitor.h"
#include "state_machine.h"
#include "steeringio.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Parameters for the pedal monitoring task */
#define MAX_ADC_VAL_12b 4096
// DEBUG: threshold may need adjusting
#define PEDAL_DIFF_THRESH 30
#define PEDAL_FAULT_TIME  500 /* ms */

static bool tsms = false;

extern mpu_t *mpu;
extern pdu_t *pdu;
extern steeringio_t *wheel;

osThreadId_t temp_monitor_handle;
const osThreadAttr_t temp_monitor_attributes = {
	.name = "TempMonitor",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityHigh1,
};

void vTempMonitor(void *pv_params)
{
	fault_data_t fault_data = { .id = ONBOARD_TEMP_FAULT, .severity = DEFCON5 };
	can_msg_t temp_msg = { .id = CANID_TEMP_SENSOR, .len = 4, .data = { 0 } };

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

osThreadId_t pedals_monitor_handle;
const osThreadAttr_t pedals_monitor_attributes = {
	.name = "PedalMonitor",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityRealtime1,
};

void eval_pedal_fault(uint16_t accel_1, uint16_t accel_2, nertimer_t *diff_timer,
					  nertimer_t *sc_timer, nertimer_t *oc_timer, fault_data_t *fault_data)
{
	/* Fault - open circuit (Max ADC value + some a lil bit) */
	if ((accel_1 > (MAX_ADC_VAL_12b - 20) || accel_2 > (MAX_ADC_VAL_12b - 20)) &&
		is_timer_active(oc_timer)) {
		if (is_timer_expired(oc_timer)) {
			if ((accel_1 == MAX_ADC_VAL_12b || accel_2 == MAX_ADC_VAL_12b)) {
				fault_data->diag = "Pedal open circuit fault - max acceleration value ";
				queue_fault(fault_data);
			}
		}
	} else if ((accel_1 > (MAX_ADC_VAL_12b - 20) || accel_2 > (MAX_ADC_VAL_12b - 20)) &&
			   !is_timer_active(oc_timer)) {
		start_timer(oc_timer, PEDAL_FAULT_TIME);
	} else {
		cancel_timer(oc_timer);
	}

	/* Fault - short circuit */
	if ((accel_1 < 500 || accel_2 < 500) && is_timer_active(sc_timer)) {
		if (is_timer_expired(sc_timer)) {
			if ((accel_1 < 500 || accel_2 < 500)) {
				fault_data->diag = "Pedal short circuit fault - no acceleration value ";
				queue_fault(fault_data);
			}
		}
	} else if ((accel_1 < 500 || accel_2 < 500) && !is_timer_active(sc_timer)) {
		start_timer(sc_timer, PEDAL_FAULT_TIME);
	} else {
		cancel_timer(sc_timer);
	}

	/* Normalize pedal values */
	uint16_t accel_1_norm =
		(uint16_t)(accel_1 - ACCEL1_OFFSET) * 100 / (ACCEL1_MAX_VAL - ACCEL1_OFFSET);
	uint16_t accel_1_clamped = accel_1 - ACCEL1_OFFSET <= 0 ? 0 : accel_1_norm;
	uint16_t accel_2_norm =
		(uint16_t)(accel_2 - ACCEL2_OFFSET) * 100 / (ACCEL2_MAX_VAL - ACCEL2_OFFSET);
	uint16_t accel_2_clamped = accel_2 - ACCEL2_OFFSET <= 0 ? 0 : accel_2_norm;

	/* Fault - difference between pedal sensing values */
	if ((abs(accel_1_clamped - accel_2_clamped) > PEDAL_DIFF_THRESH) &&
		is_timer_active(diff_timer)) {
		/* starting diff timer */
		if (is_timer_expired(diff_timer)) {
			if ((abs(accel_1_clamped - accel_2_clamped) > PEDAL_DIFF_THRESH)) {
				fault_data->diag = "Pedal fault - pedal values are too different ";
				queue_fault(fault_data);
			}
		}
	} else if ((abs(accel_1_clamped - accel_2_clamped) > PEDAL_DIFF_THRESH) &&
			   !is_timer_active(diff_timer)) {
		start_timer(diff_timer, PEDAL_FAULT_TIME);
	} else {
		cancel_timer(diff_timer);
	}
}

void vPedalsMonitor(void *pv_params)
{
	const uint8_t num_samples = 10;
	enum { ACCELPIN_2, ACCELPIN_1, BRAKEPIN_1, BRAKEPIN_2 };

	static pedals_t sensor_data;
	fault_data_t fault_data = { .id = ONBOARD_PEDAL_FAULT, .severity = DEFCON1 };
	uint32_t adc_data[4];
	bool is_braking = false;

	nertimer_t diff_timer_accelerator;
	nertimer_t sc_timer_accelerator;
	nertimer_t oc_timer_accelerator;

	cancel_timer(&diff_timer_accelerator);
	cancel_timer(&sc_timer_accelerator);
	cancel_timer(&oc_timer_accelerator);

	for (;;) {
		read_pedals(mpu, adc_data);

		/* Evaluate Pedal Faulting Conditions */
		eval_pedal_fault(adc_data[ACCELPIN_1], adc_data[ACCELPIN_2], &diff_timer_accelerator,
						 &sc_timer_accelerator, &oc_timer_accelerator, &fault_data);
		//eval_pedal_fault(adc_data[BRAKEPIN_1], adc_data[BRAKEPIN_1], &diff_timer_brake, &sc_timer_brake, &oc_timer_brake, &fault_data);

		/* Normalize pedal values */
		uint16_t accel_1_norm = (uint16_t)(adc_data[ACCELPIN_1] - ACCEL1_OFFSET) * 100 /
								(ACCEL1_MAX_VAL - ACCEL1_OFFSET);
		uint16_t accel_1_clamped = adc_data[ACCELPIN_1] - ACCEL1_OFFSET <= 0 ? 0 : accel_1_norm;
		uint16_t accel_2_norm = (uint16_t)(adc_data[ACCELPIN_2] - ACCEL2_OFFSET) * 100 /
								(ACCEL2_MAX_VAL - ACCEL2_OFFSET);
		uint16_t accel_2_clamped = adc_data[ACCELPIN_2] - ACCEL2_OFFSET <= 0 ? 0 : accel_2_norm;

		uint16_t accel_val = (uint16_t)(accel_1_clamped + accel_2_clamped) / 2;
		//printf("Avg Pedal Val: %d\r\n\n", accel_val);

		/* Brakelight Control */
		//printf("Brake 1: %ld\r\n", adc_data[BRAKEPIN_1]);
		//printf("Brake 2: %ld\r\n", adc_data[BRAKEPIN_2]);

		is_braking = (adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2 > 650;

		osMessageQueuePut(brakelight_signal, &is_braking, 0U, 0U);

		/* Low Pass Filter */
		sensor_data.accelerator_value = (sensor_data.accelerator_value + (accel_val)) / num_samples;
		sensor_data.brake_value =
			(sensor_data.brake_value + (adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2) /
			num_samples;

		/* Publish to Onboard Pedals Queue */
		osStatus_t check = osMessageQueuePut(pedal_data_queue, &sensor_data, 0U, 0U);

		if (check != 0) {
			fault_data.diag = "Failed to push pedal data to queue";
			queue_fault(&fault_data);
		}
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
	can_msg_t imu_accel_msg = { .id = CANID_IMU, .len = 6, .data = { 0 } };
	can_msg_t imu_gyro_msg = { .id = CANID_IMU, .len = 6, .data = { 0 } };

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
		sensor_data.accel_x = (sensor_data.accel_x + accel_data[0]) / num_samples;
		sensor_data.accel_y = (sensor_data.accel_y + accel_data[1]) / num_samples;
		sensor_data.accel_z = (sensor_data.accel_z + accel_data[2]) / num_samples;
		sensor_data.gyro_x = (sensor_data.gyro_x + gyro_data[0]) / num_samples;
		sensor_data.gyro_y = (sensor_data.gyro_y + gyro_data[1]) / num_samples;
		sensor_data.gyro_z = (sensor_data.gyro_z + gyro_data[2]) / num_samples;

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
	.name = "FusingMonitor",
	.stack_size = 64 * 8,
	.priority = (osPriority_t)osPriorityAboveNormal1,
};

void vFusingMonitor(void *pv_params)
{
	fault_data_t fault_data = { .id = FUSE_MONITOR_FAULT, .severity = DEFCON5 };
	can_msg_t fuse_msg = { .id = CANID_FUSE, .len = 2, .data = { 0 } };
	bool fuses[MAX_FUSES] = { 0 };
	uint16_t fuse_buf;

	for (;;) {
		fuse_buf = 0;

		for (fuse_t fuse = 0; fuse < MAX_FUSES; fuse++) {
			if (read_fuse(pdu, fuse, &fuses[fuse])) /* Actually read the fuse */
				fuse_buf |= 1 << MAX_FUSES; /* Set error bit */

			/* Sets the bit at position `fuse` to the state of the fuse */
			fuse_buf |= fuses[fuse] << fuse;
		}

		// serial_print("Fuses:\t%X\r\n", fuse_buf);

		/* convert to big endian */
		// endian_swap(&fuse_buf, sizeof(fuse_buf));

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
	.name = "ShutdownMonitor",
	.stack_size = 64 * 8,
	.priority = (osPriority_t)osPriorityHigh2,
};

void vShutdownMonitor(void *pv_params)
{
	fault_data_t fault_data = { .id = SHUTDOWN_MONITOR_FAULT, .severity = DEFCON5 };
	can_msg_t shutdown_msg = { .id = CANID_SHUTDOWN_LOOP, .len = 2, .data = { 0 } };
	bool shutdown_loop[MAX_SHUTDOWN_STAGES] = { 0 };
	uint16_t shutdown_buf;
	bool tsms_status = false;

	for (;;) {
		shutdown_buf = 0;

		for (shutdown_stage_t stage = 0; stage < MAX_SHUTDOWN_STAGES; stage++) {
			/* Actually read the shutdown loop stage state */
			if (read_shutdown(pdu, stage, &shutdown_loop[stage]))
				shutdown_buf |= 1 << MAX_SHUTDOWN_STAGES; /* Set error bit */

			/* Sets the bit at position `stage` to the state of the stage */
			shutdown_buf |= shutdown_loop[stage] << stage;
		}

		// serial_print("Shutdown status:\t%X\r\n", shutdown_buf);

		/* convert to big endian */
		endian_swap(&shutdown_buf, sizeof(shutdown_buf));

		memcpy(shutdown_msg.data, &shutdown_buf, shutdown_msg.len);
		if (queue_can_msg(shutdown_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		/* If we got a reliable TSMS reading, handle transition to and out of ACTIVE */
		if (!read_tsms_sense(pdu, &tsms_status)) {
			tsms = tsms_status;
			if (get_func_state() == ACTIVE && tsms == 0) {
				set_home_mode();
			}
		}

		// serial_print("TSMS: %d\r\n", tsms_status);

		osDelay(SHUTDOWN_MONITOR_DELAY);
	}
}

osThreadId steeringio_buttons_monitor_handle;
const osThreadAttr_t steeringio_buttons_monitor_attributes = {
	.name = "SteeringIOButtonsMonitor",
	.stack_size = 200 * 8,
	.priority = (osPriority_t)osPriorityAboveNormal1,
};

void vSteeringIOButtonsMonitor(void *pv_params)
{
	button_data_t buttons;
	can_msg_t msg = { .id = 0x680, .len = 8, .data = { 0 } };
	fault_data_t fault_data = { .id = BUTTONS_MONITOR_FAULT, .severity = DEFCON5 };

	for (;;) {
		uint8_t button_1 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
		uint8_t button_2 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
		uint8_t button_3 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
		uint8_t button_4 = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
		uint8_t button_5 = !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4);
		uint8_t button_6 = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5);
		uint8_t button_7 = !HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
		uint8_t button_8 = !HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);

		//serial_print("%d, %d, %d, %d, %d, %d, %d, %d \r\n", button_1, button_2, button_3, button_4, button_5, button_6, button_7, button_8);

		//  1, 2, 0 ,0 , 0, 0

		// serial_print("\r\n");

		uint8_t button_data = (button_1 << 7) | (button_2 << 6) | (button_3 << 5) |
							  (button_4 << 4) | (button_5 << 3) | (button_6 << 2) |
							  (button_7 << 1) | (button_8);

		buttons.data[0] = button_data;

		steeringio_update(wheel, buttons.data);

		/* Set the first byte to be the first 8 buttons with each bit representing the pin status */
		msg.data[0] = button_data;
		if (queue_can_msg(msg)) {
			fault_data.diag = "Failed to send steering buttons can message";
			queue_fault(&fault_data);
		}

		osDelay(25);
	}
}

bool get_tsms()
{
	return tsms;
}

osThreadId brakelight_monitor_handle;
const osThreadAttr_t brakelight_monitor_attributes = {
	.name = "BrakelightMonitor",
	.stack_size = 32 * 8,
	.priority = (osPriority_t)osPriorityHigh,
};

void vBrakelightMonitor(void *pv_params)
{
	bool state;
	osStatus_t status;

	for (;;) {
		status = osMessageQueueGet(brakelight_signal, &state, NULL, osWaitForever);
		if (!status) {
			write_brakelight(pdu, state);
		}
	}
}