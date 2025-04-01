#ifndef BMS_H
#define BMS_H

#include "cmsis_os.h"
#include "can.h"

#define BMS_CANID_CELL_TEMPS 0x84 /* BMS CELL TEMPERATURES */

#define BMS_DCL_MSG 0x156 /* BMS MONITOR WATCHDOG */

/**
 * @brief Callback for when a DCL message is received from the BMS.
 */
void handle_dcl_msg();

typedef struct {
	uint16_t battbox_temp;
	osMutexId_t *mutex;
} bms_t;

/*
* @brief Initializes the BMS struct and mutex
*/
void bms_init();

/*
* @brief Gets the BMS struct
* @return Pointer to the BMS struct
*/
bms_t *bms_get();

/*
* @brief Gets the current battery box temperature
* @return Battery box temperature (degrees Celsius)
*/
uint16_t bms_get_battbox_temp();

/*
 * @brief Gets the current battery box temperature
 * @return int32_t Battery box temperature
 */
void bms_record_battbox_temp(can_msg_t msg);

#endif /*BMS_H*/