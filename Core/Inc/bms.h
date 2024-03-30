
#ifndef BMS_H
#define BMS_H

#include "cmsis_os.h"

#define BMS_CANID		0x070 /*BMS MONITOR WATCHDOG*/ /*Arbitrary*/

typedef struct 
{
    osTimerId bms_monitor_timer;
} bms_t;

bms_t* bms_init();

void vBMSCANMonitor(void* pv_params);
extern osThreadId_t bms_monitor_handle;
extern const osThreadAttr_t bms_monitor_attributes;
extern osMessageQueueId_t bms_monitor_queue;

#endif /*BMS_H*/