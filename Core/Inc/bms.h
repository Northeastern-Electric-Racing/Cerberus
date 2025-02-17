#ifndef BMS_H
#define BMS_H

#include "cmsis_os.h"

#define BMS_DCL_MSG 0x156 /* BMS MONITOR WATCHDOG */

typedef struct {
	osTimerId bms_monitor_timer;
} bms_t;

/**
 * @brief Initialize BMS.
 * 
 */
bms_t *bms_init();

/**
 * @brief Callback for when a DCL message is received from the BMS.
 */
void handle_dcl_msg(bms_t *bms);

#endif /*BMS_H*/