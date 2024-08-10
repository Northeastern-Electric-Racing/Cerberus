#ifndef CERBERUS_MONITOR_H
#define CERBERUS_MONITOR_H

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "stdbool.h"
#include "mpu.h"
#include "pdu.h"
#include "steeringio.h"

typedef struct {
	uint16_t brake_value;
	uint16_t accelerator_value; /* 0-100 */
} pedals_t;

pedals_t get_pedal_data();

bool get_brake_state();

/**
 * @brief Read the open cell voltage of the LV batteries and send a CAN message with the result.
 * 
 * @param arg Pointer to mpu_t.
 */
void read_lv_sense(void *arg);

/**
 * @brief Read data from the fuse monitor GPIO expander on the PDU.
 * 
 * @param pdu Pointer to pdu_t.
 */
void read_fuse_data(void *arg);

/**
 * @brief Get the debounced TSMS reading.
 * 
 * @return State of TSMS.
 */
bool get_tsms();

/* Defining Temperature Monitor Task */
void vTempMonitor(void *pv_params);
extern osThreadId_t temp_monitor_handle;
extern const osThreadAttr_t temp_monitor_attributes;

/* Task for Petting the Watchdog */
void vWatchdogMonitor(void *pv_params);
extern osThreadId_t watchdog_monitor_handle;
extern const osThreadAttr_t watchdog_monitor_attributes;

/* Task for Reading in Pedal Inputs (Brakes + Accelerator) */
void vPedalsMonitor(void *pv_params);
extern osThreadId_t pedals_monitor_handle;
extern const osThreadAttr_t pedals_monitor_attributes;

/* Task for Monitoring the IMU */
void vIMUMonitor(void *pv_params);
extern osThreadId_t imu_monitor_handle;
extern const osThreadAttr_t imu_monitor_attributes;

/* Task for Monitoring the Shutdown Loop */
void vShutdownMonitor(void *pv_params);
extern osThreadId_t shutdown_monitor_handle;
extern const osThreadAttr_t shutdown_monitor_attributes;

/* Arguments for the data collection thread */
typedef struct {
	pdu_t *pdu;
	steeringio_t *wheel;
} data_collection_args_t;

/* Task for collecting data from the car */
void vDataCollection(void *pv_params);
extern osThreadId_t data_collection_thread;
extern const osThreadAttr_t data_collection_attributes;

#endif // MONITOR_H
