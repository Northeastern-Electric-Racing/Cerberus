#ifndef CERBERUS_MONITOR_H
#define CERBERUS_MONITOR_H

#include "cmsis_os.h"

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

#endif // MONITOR_H
