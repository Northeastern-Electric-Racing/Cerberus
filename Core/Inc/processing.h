/**
 * @file processing.h
 * @author Scott Abramson
 * @brief Functions and tasks for processing data.
 * @version 0.1
 * @date 2024-08-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef PROCESSING_H
#define PROCESSING_H

#include "cmsis_os.h"
#include "stdbool.h"

#define TSMS_UPDATE_FLAG 0x00000001U

/**
 * @brief Get the debounced TSMS reading.
 * 
 * @return State of TSMS.
 */
bool get_tsms();

/**
 * @brief Task for processing data from the TSMS.
 */
void vProcessTSMS(void *pv_params);
extern osThreadId_t process_tsms_thread_id;
extern const osThreadAttr_t process_tsms_attributes;

#endif
