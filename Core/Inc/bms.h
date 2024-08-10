
#ifndef BMS_H
#define BMS_H

#include "cmsis_os.h"

#define BMS_DCL_MSG	   0x156 /*BMS MONITOR WATCHDOG*/
#define BMS_CURR_LIMIT_MSG 0x176
#define NEW_AMS_MSG_FLAG   1U

typedef struct {
	osTimerId bms_monitor_timer;
} bms_t;

extern bms_t *bms;

void bms_init();

/**
 * @brief Callback for when a DCL message is received from the AMS.
 */
void handle_dcl_msg();

#endif /*BMS_H*/