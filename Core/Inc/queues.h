#ifndef CERBERUS_QUEUES_H
#define CERBERUS_QUEUES_H

#include "cerberus_conf.h"
#include "cmsis_os.h"

#define ONBOARD_TEMP_QUEUE_SIZE 8
#define PEDAL_DATA_QUEUE_SIZE	8
#define IMU_QUEUE_SIZE		8

extern osMessageQueueId_t brakelight_signal;

typedef struct {
	uint16_t accel_x;
	uint16_t accel_y;
	uint16_t accel_z;
	uint16_t gyro_x;
	uint16_t gyro_y;
	uint16_t gyro_z;
} imu_data_t;

extern osMessageQueueId_t imu_queue;

typedef struct {
	uint16_t brake_value; /* 0-1 */
	uint16_t accelerator_value; /* 0-1 */
} pedals_t;

extern osMessageQueueId_t pedal_data_queue;

extern osMessageQueueId_t break_state_queue;

bool get_brake_state();

#endif // QUEUES_H
