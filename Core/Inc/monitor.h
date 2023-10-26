#ifndef CERBERUS_MONITOR_H
#define CERBERUS_MONITOR_H

#include "cmsis_os.h"

/* Defining Temperature Monitor Task */
void vTempMonitor(void *pv_params);

/**
 * @brief Pet the watchdog
 * @param pv_params GPIO_TypeDef pointer to the watchdog pin
 */
void vWatchdogMonitor(void *pv_params);

/**
 * @brief IMU Monitor Task
 * @param pv_params i2c handler for the IMU
 */
void vIMUMonitor(void *pv_params);

extern osThreadId_t temp_monitor_handle;
extern const osThreadAttr_t temp_monitor_attributes;

extern osThreadId_t watchdog_monitor_handle;
extern const osThreadAttr_t watchdog_monitor_attributes;

extern osThreadId_t imu_monitor_handle;
extern const osThreadAttr_t imu_monitor_attributes;

#endif // MONITOR_H