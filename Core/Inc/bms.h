
#ifndef BMS_H
#define BMS_H

#include "cmsis_os.h"

#define BMS_DCL_MSG	   0x156 /*BMS MONITOR WATCHDOG*/
#define BMS_CURR_LIMIT_MSG 0x176

typedef struct {
	osTimerId bms_monitor_timer;
	uint16_t dcl;
	uint16_t ccl;
} bms_t;

extern bms_t *bms;

void bms_init();

/**
 * @brief Task for handling CAN messages received from the AMS.
 */
void vAMSCANMonitor(void *pv_params);
extern osThreadId_t bms_monitor_handle;
extern const osThreadAttr_t bms_monitor_attributes;
extern osMessageQueueId_t bms_monitor_queue;

#endif /*BMS_H*/