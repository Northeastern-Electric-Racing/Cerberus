#ifndef CERBERUS_MONITOR_H
#define CERBERUS_MONITOR_H

#include <stdbool.h>

#include "cmsis_os_test.h"
#include "mpu.h"
#include "pdu.h"

/**
 * @brief Get the debounced TSMS reading.
 *
 * @return State of TSMS.
 */
bool get_tsms();

typedef struct {
	mpu_t *mpu;
	pdu_t *pdu;
} non_func_data_args_t;

/**
 * @brief Task for collecting non functional data from the car. Includes LV battery voltage reading
 * and fuse monitor reading.
 *
 * @param pv_params Pointer to non_func_data_args_t
 */
void vNonFunctionalDataCollection(void *pv_params);
extern osThreadId_t non_functional_data_thead;
extern const osThreadAttr_t non_functional_data_attributes;

/* Arguments for the data collection thread */
typedef struct {
	pdu_t *pdu;
} data_collection_args_t;

/**
 * @brief Task for collecting functional data from the car. Includes TSMS and steering wheel button
 * monitoring.
 *
 * @param pv_params Pointer to data_collection_args_t
 */
void vDataCollection(void *pv_params);
extern osThreadId_t data_collection_thread;
extern const osThreadAttr_t data_collection_attributes;

/* Unused
 * -------------------------------------------------------------------------------------------------------*/

/* Task for Monitoring the IMU */
// void vIMUMonitor(void *pv_params);
// extern osThreadId_t imu_monitor_handle;
// extern const osThreadAttr_t imu_monitor_attributes;

/* Defining Temperature Monitor Task */
// void vTempMonitor(void *pv_params);
// extern osThreadId_t temp_monitor_handle;
// extern const osThreadAttr_t temp_monitor_attributes;

#endif // MONITOR_H
