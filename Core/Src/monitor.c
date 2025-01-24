#include "monitor.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cerb_utils.h"
#include "cerberus_conf.h"
#include "fault.h"
#include "state_machine.h"

#define TSMS_DEBOUNCE_PERIOD 500 /* ms */

static bool tsms = false;
osMutexId_t tsms_mutex;

/**
 * @brief Read voltage of Pump Sensors and send a CAN message with the result.
 */
void read_pump_sens(pdu_t *pdu)
{
	// put fault stuff tomorrow
	fault_data_t fault_data = { .id = PUMP_SENSORS_FAULT,
				    .severity = DEFCON5 };
	can_msg_t msg = { .id = CANID_PDU_CURRENT, .len = 8, .data = { 0 } };

	uint32_t pump_volts_int[2];

	read_pump_sensors(pdu, pump_volts_int);

	// convert to real voltage
	float pump_sensor0_volts_real = (pump_volts_int[0] / 4095.0) * 3.3;
	float pump_sensor1_volts_real = (pump_volts_int[1] / 4095.0) * 3.3;

	pump_volts_int[0] = (uint32_t)(pump_sensor0_volts_real * 10000);
	pump_volts_int[1] = (uint32_t)(pump_sensor1_volts_real * 10000);

	memcpy(msg.data, pump_volts_int, msg.len);
	if (queue_can_msg(msg)) {
		fault_data.diag = "Failed to send pump sensor CAN message";
		queue_fault(&fault_data);
	}
}

/**
 * @brief Read current of MC, battox fans, pumps, and LV boards and send a CAN message with the result.
 */
void read_current(pdu_t *pdu)
{
	fault_data_t fault_data = { .id = PDU_CURRENT_FAULT,
				    .severity = DEFCON5 };
	can_msg_t msg = { .id = CANID_PDU_CURRENT, .len = 8, .data = { 0 } };

	float motor_controller_current;
	float battbox_fans_current;
	float pumps_current;
	float lv_boards_current;

	if (read_all_current(pdu, &motor_controller_current,
			     &battbox_fans_current, &pumps_current,
			     &lv_boards_current)) {
		fault_data.diag = "Failed to read current";
		queue_fault(&fault_data);
	}

	uint16_t int_motor_controller_current =
		(uint16_t)(motor_controller_current * 1000);
	uint16_t int_battbox_fans_current =
		(uint16_t)(battbox_fans_current * 1000);
	uint16_t int_pumps_current = (uint16_t)(pumps_current * 1000);
	uint16_t int_lv_boards_current = (uint16_t)(lv_boards_current * 1000);

	struct __attribute__((__packed__)) {
		uint16_t motor_controller;
		uint16_t battbox_fans;
		uint16_t pumps_current;
		uint16_t lv_boards;
	} current_data;

	current_data.motor_controller = int_motor_controller_current;
	current_data.battbox_fans = int_battbox_fans_current;
	current_data.pumps_current = int_pumps_current;
	current_data.lv_boards = int_lv_boards_current;

	memcpy(msg.data, &current_data, msg.len);
	if (queue_can_msg(msg)) {
		fault_data.diag = "Failed to send current CAN message";
		queue_fault(&fault_data);
	}
}

/**
 * @brief Read the open cell voltage of the LV batteries and send a CAN message with the result.
 */
void read_lv_sense(void *arg)
{
	mpu_t *mpu = (mpu_t *)arg;
	fault_data_t fault_data = { .id = LV_MONITOR_FAULT,
				    .severity = DEFCON5 };
	can_msg_t lv_msg = { .id = CANID_LV_MONITOR, .len = 5, .data = { 0 } };

	uint32_t v_int;
	uint32_t soc_int;

	struct __attribute__((__packed__)) {
		uint32_t v;
		uint8_t soc;
	} lv_data;

	read_lv_voltage(mpu, &v_int);

	/* Convert from raw ADC reading to voltage level */

	// scale up then truncate
	// convert to out of 24 volts
	// since 12 bits / 4096
	// Magic number bc idk the resistors on the voltage divider
	float v_dec = v_int * 8.967;

	// get final voltage
	v_int = (uint32_t)(v_dec * 10.0);

	// Calculate SoC using logistic function
	// - Normal charged voltage is 29.4V
	// - 18650 max charged voltage is 4.2V
	// - 29.4V/4.2V = 7 batteries
	// - Divide v_dec by 7 to get avg voltage over all rows
	// - Use logistic function to model SoC
	// https://www.aegisbattery.com/products/24v-20ah-li-ion-battery-pvc
	static const float v_max = 4.2; // max avg voltage over all rows (7)
	static const float v_min = 2.8; // min avg voltage over all rows (7)
	static const float k =
		-8.5; // logistic fn parameter to affect steepness of curve
	float i = (v_max + v_min) /
		  2; // logistic fn parameter to affect midpoint of curve
	float soc_dec =
		1 / (1 + exp(k * (v_dec - i))); // SoC calculation in [0,1]
	soc_int = soc_dec * 100;

	lv_data.v = v_int;
	lv_data.soc = soc_int;

	memcpy(lv_msg.data, &lv_data, lv_msg.len);
	if (queue_can_msg(lv_msg)) {
		fault_data.diag =
			"Failed to send steering LV monitor CAN message";
		queue_fault(&fault_data);
	}
}

/**
 * @brief Read data from the fuse monitor GPIO expander on the PDU and send a CAN message with the
 * resulting data.
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
const osThreadAttr_t non_functional_data_attributes = {
	.name = "NonFunctionalDataCollection",
	.stack_size = 2048,
	.priority = (osPriority_t)osPriorityBelowNormal,
};
void vNonFunctionalDataCollection(void *pv_params)
{
	non_func_data_args_t *args = (non_func_data_args_t *)pv_params;
	mpu_t *mpu = args->mpu;
	assert(mpu);
	pdu_t *pdu = args->pdu;
	assert(pdu);

	free(args);

	for (;;) {
		read_lv_sense(mpu);
		read_fuse_data(pdu);
		read_current(pdu);

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
		/* Since debounce only debounces logic high signals, the reading must be inverted if it is
		 * low. Think of this as debouncing a "TSMS off is active" debounce. */
		debounce(!tsms_reading, &timer, TSMS_DEBOUNCE_PERIOD,
			 &tsms_debounce_cb, &tsms_reading);

	if (get_active() && get_tsms() == false) {
		set_home_mode();
	}
}

osThreadId_t data_collection_thread;
const osThreadAttr_t data_collection_attributes = {
	.name = "DataCollection",
	.stack_size = 2048,
	.priority = (osPriority_t)osPriorityBelowNormal,
};

void vDataCollection(void *pv_params)
{
	data_collection_args_t *args = (data_collection_args_t *)pv_params;
	pdu_t *pdu = args->pdu;
	assert(pdu);

	free(args);

	static const uint8_t delay = 20;

	tsms_mutex = osMutexNew(NULL);

	for (;;) {
		read_tsms(pdu);
		osDelay(delay);
	}
}

/* Unused -----------------------------------------------*/

// osThreadId_t temp_monitor_handle;
// const osThreadAttr_t temp_monitor_attributes = {
// 	.name = "TempMonitor",
// 	.stack_size = 32 * 8,
// 	.priority = (osPriority_t)osPriorityHigh1,
// };

// void vTempMonitor(void *pv_params)
// {
// 	fault_data_t fault_data = { .id = ONBOARD_TEMP_FAULT,
// 				    .severity = DEFCON5 };
// 	can_msg_t temp_msg = { .id = CANID_TEMP_SENSOR,
// 			       .len = 4,
// 			       .data = { 0 } };

// 	mpu_t *mpu = (mpu_t *)pv_params;

// 	for (;;) {
// 		/* Take measurement */
// 		uint16_t temp = 0;
// 		uint16_t humidity = 0;
// 		if (read_temp_sensor(mpu, &temp, &humidity)) {
// 			fault_data.diag = "Failed to get temp";
// 			queue_fault(&fault_data);
// 		}

// 		printf("MPU Board Temperature:\t%d\n", temp);

// 		temp_msg.data[0] = temp & 0xFF;
// 		temp_msg.data[1] = (temp >> 8) & 0xFF;
// 		temp_msg.data[2] = humidity & 0xFF;
// 		temp_msg.data[3] = (humidity >> 8) & 0xFF;

// 		/* Send CAN message */
// 		if (queue_can_msg(temp_msg)) {
// 			fault_data.diag = "Failed to send CAN message";
// 			queue_fault(&fault_data);
// 		}

// 		/* Yield to other tasks */
// 		osDelay(TEMP_SENS_SAMPLE_DELAY);
// 	}
// }

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
		shutdown_data.shut_1 = reverse_bits(shutdown_data.shut_1);
		shutdown_data.shut_2 = reverse_bits(shutdown_data.shut_2);

		memcpy(shutdown_msg.data, &shutdown_data, shutdown_msg.len);
		if (queue_can_msg(shutdown_msg)) {
			fault_data.diag = "Failed to send CAN message";
			queue_fault(&fault_data);
		}

		osDelay(SHUTDOWN_MONITOR_DELAY);
	}
}

// osThreadId_t imu_monitor_handle;
// const osThreadAttr_t imu_monitor_attributes = {
// 	.name = "IMUMonitor",
// 	.stack_size = 32 * 8,
// 	.priority = (osPriority_t)osPriorityHigh,
// };

// void vIMUMonitor(void *pv_params)
// {
// 	const uint8_t num_samples = 10;
// 	static imu_data_t sensor_data;
// 	fault_data_t fault_data = { .id = IMU_FAULT, .severity = DEFCON5 };
// 	can_msg_t imu_accel_msg = { .id = CANID_IMU_ACCEL,
// 				    .len = 6,
// 				    .data = { 0 } };
// 	can_msg_t imu_gyro_msg = { .id = CANID_IMU_GYRO,
// 				   .len = 6,
// 				   .data = { 0 } };

// 	mpu_t *mpu = (mpu_t *)pv_params;

// 	for (;;) {
// 		// printf("IMU Task\r\n");
// 		/* Take measurement */
// 		if (read_accel(mpu)) {
// 			fault_data.diag = "Failed to get IMU acceleration";
// 			queue_fault(&fault_data);
// 		}

// 		if (read_gyro(mpu)) {
// 			fault_data.diag = "Failed to get IMU gyroscope";
// 			queue_fault(&fault_data);
// 		}

// 		/* Run values through LPF of sample size  */
// 		sensor_data.accel_x =
// 			(sensor_data.accel_x + mpu->imu->accel_data[0]) /
// 			num_samples;
// 		sensor_data.accel_y =
// 			(sensor_data.accel_y + mpu->imu->accel_data[1]) /
// 			num_samples;
// 		sensor_data.accel_z =
// 			(sensor_data.accel_z + mpu->imu->accel_data[2]) /
// 			num_samples;
// 		sensor_data.gyro_x =
// 			(sensor_data.gyro_x + mpu->imu->gyro_data[0]) /
// 			num_samples;
// 		sensor_data.gyro_y =
// 			(sensor_data.gyro_y + mpu->imu->gyro_data[1]) /
// 			num_samples;
// 		sensor_data.gyro_z =
// 			(sensor_data.gyro_z + mpu->imu->gyro_data[2]) /
// 			num_samples;

// 		/* Publish to IMU Queue */
// 		osMessageQueuePut(imu_queue, &sensor_data, 0U, 0U);

// 		/* convert to big endian */
// 		endian_swap(&sensor_data.accel_x, sizeof(sensor_data.accel_x));
// 		endian_swap(&sensor_data.accel_y, sizeof(sensor_data.accel_y));
// 		endian_swap(&sensor_data.accel_z, sizeof(sensor_data.accel_z));
// 		endian_swap(&sensor_data.gyro_x, sizeof(sensor_data.gyro_x));
// 		endian_swap(&sensor_data.gyro_y, sizeof(sensor_data.gyro_y));
// 		endian_swap(&sensor_data.gyro_z, sizeof(sensor_data.gyro_z));

// 		/* Send CAN message */
// 		memcpy(imu_accel_msg.data, &sensor_data, imu_accel_msg.len);
// 		// if (queue_can_msg(imu_accel_msg)) {
// 		//	fault_data.diag = "Failed to send CAN message";
// 		//	queue_fault(&fault_data);
// 		// }

// 		memcpy(imu_gyro_msg.data, &sensor_data, imu_gyro_msg.len);
// 		if (queue_can_msg(imu_gyro_msg)) {
// 			fault_data.diag = "Failed to send CAN message";
// 			queue_fault(&fault_data);
// 		}

// 		/* Yield to other tasks */
// 		osDelay(IMU_SAMPLE_DELAY);
// 	}
// }
