
#ifndef BMS_CAN_MONITOR_H
#define BMS_CAN_MONITOR_H

#include "cmsis_os.h"
#include "timer.h"

#define CANID_BMS_MONITOR		0x070 /*BMS MONITOR WATCHDOG*/ /*Arbitrary*/

typedef struct 
{
    nertimer_t timer;
} bms_can_monitor_t;


bms_can_monitor_t* bms_can_monitor_init();

void vBMSCANMonitor(void* pv_params);
extern osThreadId_t bms_can_monitor_handle;
extern const osThreadAttr_t bms_can_monitor_attributes;
extern osMessageQueueId_t bms_can_monitor_queue;

#endif /*BMS_CAN_MONITOR_H*/