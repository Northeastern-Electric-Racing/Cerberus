#ifndef CERBERUS_MONITOR_H
#define CERBERUS_MONITOR_H

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

/* Defining Temperature Monitor Task */
void vTempMonitor(void *pv_params);
extern osThreadId_t temp_monitor_handle;
extern const osThreadAttr_t temp_monitor_attributes;

/* Task for Petting the Watchdog */
void vWatchdogMonitor(void *pv_params);
extern osThreadId_t watchdog_monitor_handle;
extern const osThreadAttr_t watchdog_monitor_attributes;

typedef struct
{
    ADC_HandleTypeDef *accel_adc1;
    ADC_HandleTypeDef *accel_adc2;
    ADC_HandleTypeDef *brake_adc;
} pedal_params_t;

/* Parameters for the pedal monitoring task */
#define MAX_ADC_VAL_12B 4096
#define PEDAL_DIFF_THRESH   10
#define PEDAL_FAULT_TIME    1000 /* ms */

/* Task for Reading in Pedal Inputs (Brakes + Accelerator) */
void vPedalsMonitor(void *pv_params);
extern osThreadId_t pedals_monitor_handle;
extern const osThreadAttr_t pedals_monitor_attributes;

/* Task for Monitoring the IMU */
void vIMUMonitor(void *pv_params);
extern osThreadId_t imu_monitor_handle;
extern const osThreadAttr_t imu_monitor_attributes;

/* Task for Monitoring the Fuses on PDU */
void vFusingMonitor(void *pv_params);
extern osThreadId_t fusing_monitor_handle;
extern const osThreadAttr_t fusing_monitor_attributes;

/* Task for Monitoring the Shutdown Loop */
void vShutdownMonitor(void *pv_params);
extern osThreadId_t shutdown_monitor_handle;
extern const osThreadAttr_t shutdown_monitor_attributes;

#endif // MONITOR_H
