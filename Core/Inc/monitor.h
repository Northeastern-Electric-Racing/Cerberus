#ifndef CERBERUS_MONITOR_H
#define CERBERUS_MONITOR_H

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "stdbool.h"

/**
 * @brief Task monitoring the voltage level of the low voltage batteries.
 */
void vLVMonitor(void *pv_params);
extern osThreadId lv_monitor_handle;
extern const osThreadAttr_t lv_monitor_attributes;

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

/* Task for Monitoring the TSMS sense on PDU  CTRL expander*/
void vTsmsMonitor(void *pv_params);
extern osThreadId_t tsms_monitor_handle;
extern const osThreadAttr_t tsms_monitor_attributes;

/* Task for Monitoring the Fuses on PDU */
void vFusingMonitor(void *pv_params);
extern osThreadId_t fusing_monitor_handle;
extern const osThreadAttr_t fusing_monitor_attributes;

/* Task for Monitoring the Shutdown Loop */
void vShutdownMonitor(void *pv_params);
extern osThreadId_t shutdown_monitor_handle;
extern const osThreadAttr_t shutdown_monitor_attributes;

void vSteeringIOButtonsMonitor(void *pv_params);
extern osThreadId_t steeringio_buttons_monitor_handle;
extern const osThreadAttr_t steeringio_buttons_monitor_attributes;

void vBrakelightMonitor(void *pv_params);
extern osThreadId brakelight_monitor_handle;
extern const osThreadAttr_t brakelight_monitor_attributes;

#endif // MONITOR_H
