
#ifndef AMS_H
#define AMS_H

#include "cmsis_os.h"

#define AMS_DCL_MSG 0x156 /* AMS MONITOR WATCHDOG*/

typedef struct {
	osTimerId ams_monitor_timer;
} ams_t;

extern ams_t *ams;

/**
 * @brief Initialize AMS.
 * 
 */
void ams_init();

/**
 * @brief Callback for when a DCL message is received from the AMS.
 */
void handle_dcl_msg();

#endif /*AMS_H*/