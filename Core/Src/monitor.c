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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parameters for the pedal monitoring task */
#define MAX_ADC_VAL_12b 4096
// DEBUG: threshold may need adjusting
#define PEDAL_DIFF_THRESH 30
#define PEDAL_FAULT_TIME  500 /* ms */
 

static bool tsms = false;
static bool brake_state = false;


osThreadId lv_monitor_handle;
const osThreadAttr_t lv_monitor_attributes = {
	.name		= "LVMonitor",
	.stack_size = 32 * 8,
	.priority	= (osPriority_t)osPriorityHigh4,
};

void vLVMonitor(void* pv_params)
{
	mpu_t* mpu = (mpu_t*)pv_params;
	fault_data_t fault_data = { .id = LV_MONITOR_FAULT, .severity = DEFCON5 };
	can_msg_t msg = { .id = CANID_LV_MONITOR, .len = 4, .data = { 0 } };

	uint32_t v_int;

	for (;;) {
		read_lv_voltage(mpu, &v_int);

		// scale up then truncate
		// convert to out of 24 volts
		// since 12 bits / 4096
		// Magic number bc idk the resistors on the voltage divider
		float v_dec = v_int * 8.967;

		// get final voltage
		v_int = (uint32_t) (v_dec *10.0);

		//endian_swap(&v_int, sizeof(v_int));
		memcpy(msg.data, &v_int, msg.len);
		if (queue_can_msg(msg)) {
			fault_data.diag = "Failed to send steering buttons can message";
			queue_fault(&fault_data);
		}

		osDelay(LV_READ_DELAY);
	}
}

osThreadId_t temp_monitor_handle;
const osThreadAttr_t temp_monitor_attributes = {
	.name		= "TempMonitor",
	.stack_size = 32 * 8,
	.priority	= (osPriority_t)osPriorityHigh1,
};

bool get_brake_state()
{
	return brake_state;
}

void vTempMonitor(void* pv_params)
{
	fault_data_t fault_data = { .id = ONBOARD_TEMP_FAULT, .severity = DEFCON5 };
	can_msg_t temp_msg		= { .id = CANID_TEMP_SENSOR, .len = 4, .data = { 0 } };

	mpu_t* mpu = (mpu_t*)pv_params;

	for (;;) {
		/* Take measurement */
		uint16_t temp	  = 0;
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
	.name		= "PedalMonitor",
	.stack_size = 128 * 8,
	.priority	= (osPriority_t)osPriorityRealtime1,
};

void eval_pedal_fault(uint16_t accel_1, uint16_t accel_2, nertimer_t* diff_timer,
					  nertimer_t* sc_timer, nertimer_t* oc_timer, fault_data_t* fault_data)
{
	/* Fault - open circuit (Max ADC value + some a lil bit) */
	if ((accel_1 > (MAX_ADC_VAL_12b - 20) || accel_2 > (MAX_ADC_VAL_12b - 20))
		&& is_timer_active(oc_timer)) {
		if (is_timer_expired(oc_timer)) {
			if ((accel_1 == MAX_ADC_VAL_12b || accel_2 == MAX_ADC_VAL_12b)) {
				fault_data->diag = "Pedal open circuit fault - max acceleration value ";
				queue_fault(fault_data);
			}
		}
	} else if ((accel_1 > (MAX_ADC_VAL_12b - 20) || accel_2 > (MAX_ADC_VAL_12b - 20))
			   && !is_timer_active(oc_timer)) {
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
	uint16_t accel_1_norm = accel_1 - ACCEL1_OFFSET <= 0 ? 0 : (uint16_t)(accel_1 - ACCEL1_OFFSET) * 100 / (ACCEL1_MAX_VAL - ACCEL1_OFFSET);
	uint16_t accel_2_norm = accel_2 - ACCEL2_OFFSET <= 0 ? 0 : (uint16_t)(accel_2 - ACCEL2_OFFSET) * 100 / (ACCEL2_MAX_VAL - ACCEL2_OFFSET);

	/* Fault - difference between pedal sensing values */
	if ((abs(accel_1_norm - accel_2_norm) > PEDAL_DIFF_THRESH) && is_timer_active(diff_timer)) {
	
		/* starting diff timer */
		if (is_timer_expired(diff_timer)) {
			if ((abs(accel_1_norm - accel_2_norm) > PEDAL_DIFF_THRESH)) {
				fault_data->diag = "Pedal fault - pedal values are too different ";
				queue_fault(fault_data);
			}
		}
	} else if ((abs(accel_1_norm - accel_2_norm) > PEDAL_DIFF_THRESH)
			   && !is_timer_active(diff_timer)) {
		start_timer(diff_timer, PEDAL_FAULT_TIME);
	} else {
		cancel_timer(diff_timer);
	}
}

void vPedalsMonitor(void* pv_params)
{
	const uint8_t num_samples = 10;
	enum { ACCELPIN_2, ACCELPIN_1, BRAKEPIN_1, BRAKEPIN_2 };

	static pedals_t sensor_data;
	fault_data_t fault_data	   = { .id = ONBOARD_PEDAL_FAULT, .severity = DEFCON1 };
	can_msg_t accel_pedals_msg = { .id = CANID_PEDALS_ACCEL_MSG, .len = 8, .data = { 0 } };
	can_msg_t brake_pedals_msg = { .id = CANID_PEDALS_BRAKE_MSG, .len = 8, .data = { 0 } };
	uint32_t adc_data[4];
	bool is_braking = false;

	nertimer_t diff_timer_accelerator;
	nertimer_t sc_timer_accelerator;
	nertimer_t oc_timer_accelerator;

	cancel_timer(&diff_timer_accelerator);
	cancel_timer(&sc_timer_accelerator);
	cancel_timer(&oc_timer_accelerator);

	/* Handle ADC Data for two input accelerator value and two input brake value*/
	mpu_t* mpu = (mpu_t*)pv_params;

	uint8_t counter = 0;

	for (;;) {
		read_pedals(mpu, adc_data);

		/* Evaluate Pedal Faulting Conditions */
		eval_pedal_fault(adc_data[ACCELPIN_1], adc_data[ACCELPIN_2], &diff_timer_accelerator, &sc_timer_accelerator, &oc_timer_accelerator, &fault_data);
		//eval_pedal_fault(adc_data[BRAKEPIN_1], adc_data[BRAKEPIN_1], &diff_timer_brake, &sc_timer_brake, &oc_timer_brake, &fault_data);
		
		/* Offset adjusted per pedal sensor, clamp to be above 0 */
		uint16_t accel_val1 = (int16_t)adc_data[ACCELPIN_1] - ACCEL1_OFFSET <= 0 ? 0 : (uint16_t)(adc_data[ACCELPIN_1] - ACCEL1_OFFSET) * 100 / (ACCEL1_MAX_VAL - ACCEL1_OFFSET);
		// printf("Accel 1: %d\r\n", max_pedal1);
		uint16_t accel_val2 = (int16_t)adc_data[ACCELPIN_2] - ACCEL2_OFFSET <= 0 ? 0 : (uint16_t)(adc_data[ACCELPIN_2] - ACCEL2_OFFSET) * 100 / (ACCEL2_MAX_VAL - ACCEL2_OFFSET);
		// printf("Accel 2: %d\r\n",max_pedal2);

		uint16_t accel_val = (uint16_t)(accel_val1 + accel_val2) / 2;
		// printf("Avg Pedal Val: %d\r\n\n", accel_val);

		/* Raw ADC for tuning */
		//printf("Accel 1: %ld\r\n", adc_data[ACCELPIN_1]);
		//printf("Accel 2: %ld\r\n", adc_data[ACCELPIN_2]);

		/* Brakelight Control */
		// printf("Brake 1: %ld\r\n", adc_data[BRAKEPIN_1]);
		// printf("Brake 2: %ld\r\n", adc_data[BRAKEPIN_2]);

		is_braking = (adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2 > PEDAL_BRAKE_THRESH;
		brake_state = is_braking;

		osMessageQueuePut(brakelight_signal, &is_braking, 0U, 0U);
		//osMessageQueueReset(break_state_queue);
		//osMessageQueuePut(break_state_queue, &is_braking, 0U, 0U);

		/* Low Pass Filter */
		sensor_data.accelerator_value = (sensor_data.accelerator_value + (accel_val)) / num_samples;
		sensor_data.brake_value
			= (sensor_data.brake_value + (adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2)
			  / num_samples;

		/* Publish to Onboard Pedals Queue */
		osStatus_t check = osMessageQueuePut(pedal_data_queue, &sensor_data, 0U, 0U);

		if (check != 0) {
			fault_data.diag = "Failed to push pedal data to queue";
			queue_fault(&fault_data);
		}

		/* Send CAN messages with raw pedal readings, we do not care if it fails*/
		counter += 1;
		if (counter >= 5) {
			counter = 0;
			endian_swap(&adc_data[ACCELPIN_1], sizeof(adc_data[ACCELPIN_1]));
			endian_swap(&adc_data[ACCELPIN_2], sizeof(adc_data[ACCELPIN_2]));
			memcpy(accel_pedals_msg.data, &adc_data, accel_pedals_msg.len);
			queue_can_msg(accel_pedals_msg);

			endian_swap(&adc_data[BRAKEPIN_1], sizeof(adc_data[BRAKEPIN_1]));
			endian_swap(&adc_data[BRAKEPIN_2], sizeof(adc_data[BRAKEPIN_2]));
			memcpy(brake_pedals_msg.data, adc_data + 2, brake_pedals_msg.len);
			queue_can_msg(brake_pedals_msg);
		}

		osDelay(PEDALS_SAMPLE_DELAY);
	}
}

osThreadId_t imu_monitor_handle;
const osThreadAttr_t imu_monitor_attributes = {
	.name		= "IMUMonitor",
	.stack_size = 32 * 8,
	.priority	= (osPriority_t)osPriorityHigh,
};

void vIMUMonitor(void* pv_params)
{
	const uint8_t num_samples = 10;
	static imu_data_t sensor_data;
	fault_data_t fault_data = { .id = IMU_FAULT, .severity = DEFCON5 };
	can_msg_t imu_accel_msg = { .id = CANID_IMU_ACCEL, .len = 6, .data = { 0 } };
	can_msg_t imu_gyro_msg	= { .id = CANID_IMU_GYRO, .len = 6, .data = { 0 } };

	mpu_t* mpu = (mpu_t*)pv_params;

	for (;;) {
		// serial_print("IMU Task\r\n");
		/* Take measurement */
		uint16_t accel_data[3] = { 0 };
		uint16_t gyro_data[3]  = { 0 };
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
		sensor_data.gyro_x	= (sensor_data.gyro_x + gyro_data[0]) / num_samples;
		sensor_data.gyro_y	= (sensor_data.gyro_y + gyro_data[1]) / num_samples;
		sensor_data.gyro_z	= (sensor_data.gyro_z + gyro_data[2]) / num_samples;

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

osThreadId_t fusing_monitor_handle;
const osThreadAttr_t fusing_monitor_attributes = {
	.name		= "FusingMonitor",
	.stack_size = 64 * 8,
	.priority	= (osPriority_t)osPriorityAboveNormal1,
};

void vFusingMonitor(void* pv_params)
{
	fault_data_t fault_data = { .id = FUSE_MONITOR_FAULT, .severity = DEFCON5 };
	can_msg_t fuse_msg		= { .id = CANID_FUSE, .len = 2, .data = { 0 } };
	pdu_t* pdu				= (pdu_t*)pv_params;
	uint16_t fuse_buf;
	bool fuses[MAX_FUSES]
		= { 0 };

	struct __attribute__((__packed__)) {
		uint8_t fuse_1;
		uint8_t fuse_2;
	} fuse_data;

	bool tsms_status = false;

	for (;;) {
		fuse_buf = 0;

		if (read_fuses(pdu, fuses)) {
			fault_data.diag = "Failed to read fuses";
			queue_fault(&fault_data);
		}

		for (fuse_t fuse = 0; fuse < MAX_FUSES; fuse++) {
			fuse_buf |= fuses[fuse]
						<< fuse; /* Sets the bit at position `fuse` to the state of the fuse */
		}

		// serial_print("Fuses:\t%X\r\n", fuse_buf);

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

		/* If we got a reliable TSMS reading, handle transition to and out of ACTIVE*/
		if (!read_tsms_sense(pdu, &tsms_status)) {
			tsms = tsms_status;
			if (get_func_state() == ACTIVE && tsms == 0) {
				set_home_mode();
			}
		}


		osDelay(FUSES_SAMPLE_DELAY);
	}
}

osThreadId_t shutdown_monitor_handle;
const osThreadAttr_t shutdown_monitor_attributes = {
	.name		= "ShutdownMonitor",
	.stack_size = 64 * 8,
	.priority	= (osPriority_t)osPriorityHigh2,
};

void vShutdownMonitor(void* pv_params)
{
	fault_data_t fault_data = { .id = SHUTDOWN_MONITOR_FAULT, .severity = DEFCON5 };
	can_msg_t shutdown_msg	= { .id = CANID_SHUTDOWN_LOOP, .len = 2, .data = { 0 } };
	pdu_t* pdu				= (pdu_t*)pv_params;
	bool shutdown_loop[MAX_SHUTDOWN_STAGES] = { 0 };
	uint16_t shutdown_buf;

	struct __attribute__((__packed__)) {
		uint8_t shut_1;
		uint8_t shut_2;
	} shutdown_data;

	for (;;) {
		shutdown_buf = 0;

		if (read_shutdown(pdu,shutdown_loop)) {
			fault_data.diag = "Failed to read shutdown buffer";
			queue_fault(&fault_data);
		}

		for (shutdown_stage_t stage = 0; stage < MAX_SHUTDOWN_STAGES; stage++) {
			shutdown_buf
				|= shutdown_loop[stage]
				   << stage; /* Sets the bit at position `stage` to the state of the stage */
		}

		// serial_print("Shutdown status:\t%X\r\n", shutdown_buf);

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

		// serial_print("TSMS: %d\r\n", tsms_status);

		osDelay(SHUTDOWN_MONITOR_DELAY);
	}
}

osThreadId steeringio_buttons_monitor_handle;
const osThreadAttr_t steeringio_buttons_monitor_attributes = {
	.name		= "SteeringIOButtonsMonitor",
	.stack_size = 200 * 8,
	.priority	= (osPriority_t)osPriorityAboveNormal1,
};

void vSteeringIOButtonsMonitor(void* pv_params)
{
	button_data_t buttons;
	steeringio_t* wheel		= (steeringio_t*)pv_params;
	can_msg_t msg			= { .id = 0x680, .len = 8, .data = { 0 } };
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

		// serial_print("%d, %d, %d, %d, %d, %d, %d, %d \r\n", button_1, button_2, button_3,
		// button_4, button_5, button_6, button_7, button_8);

		//  1, 2, 0 ,0 , 0, 0

		// serial_print("\r\n");

		uint8_t button_data = (button_1 << 7) | (button_2 << 6) | (button_3 << 5) | (button_4 << 4)
							  | (button_5 << 3) | (button_6 << 2) | (button_7 << 1) | (button_8);

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
	.name		= "BrakelightMonitor",
	.stack_size = 32 * 8,
	.priority	= (osPriority_t)osPriorityHigh,
};

void vBrakelightMonitor(void* pv_params)
{
	pdu_t* pdu = (pdu_t*)pv_params;

	bool state;
	osStatus_t status;

	for (;;) {
		status = osMessageQueueGet(brakelight_signal, &state, NULL, osWaitForever);
		if (!status) {
			write_brakelight(pdu, state);
		}
	}
}

osThreadId fan_battbox_monitor_handle;
const osThreadAttr_t fan_battbox_monitor_attributes = {
	.name		= "FanBattboxMonitor",
	.stack_size = 32 * 8,
	.priority	= (osPriority_t)osPriorityAboveNormal1,
};

void vFanBattboxMonitor(void* pv_params)
{
	pdu_t* pdu = (pdu_t*)pv_params;

	uint8_t temperature;
	osStatus_t status;

	for (;;) {
		status = osMessageQueueGet(fan_battbox_state, &temperature, NULL, osWaitForever);
		if (!status) {
			if (temperature > 35) {
				write_fan_battbox(pdu, true);
			} else {
				write_fan_battbox(pdu, false);
			}
		}
	}
}