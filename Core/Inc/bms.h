#ifndef BMS_H
#define BMS_H

#include "cmsis_os.h"

#define BMS_DCL_MSG 0x156 /* BMS MONITOR WATCHDOG */

/**
 * @brief Callback for when a DCL message is received from the BMS.
 */
void handle_dcl_msg();

/**
 * @brief initializes bms timer
 */
void init_bms();

#endif /*BMS_H*/