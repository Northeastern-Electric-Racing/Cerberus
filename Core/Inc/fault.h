#ifndef CERBERUS_FAULT_H
#define CERBERUS_FAULT_H

#include "cmsis_os.h"
#include <stdbool.h>

typedef enum {
	ONBOARD_PEDAL_FAULT = 0,
	CAN_DISPATCH_FAULT = 1,
	CAN_ROUTING_FAULT = 2,
	BMS_CAN_MONITOR_FAULT = 3,
	MAX_CRITICAL_FAULT = 4
} crit_fault_t;

// Unused Fault Message IDs
// DTI_ROUTING_FAULT
// STEERINGIO_ROUTING_FAULT
// STATE_RECEIVED_FAULT
// INVALID_TRANSITION_FAULT
// BUTTONS_MONITOR_FAULT

typedef enum {
	ONBOARD_TEMP_FAULT = 0,
	IMU_FAULT = 1,
	FUSE_MONITOR_FAULT = 2,
	SHUTDOWN_MONITOR_FAULT = 3,
	LV_MONITOR_FAULT = 4,
	BSPD_PREFAULT = 5,
	RTDS_FAULT = 6,
	MAX_NON_CRITICAL_FAULT = 7
} non_crit_fault_t;

typedef enum { CRITICAL, NONCRITICAL } severity_t;

typedef struct {
	severity_t severity;
	union {
		crit_fault_t crit_fault;
		non_crit_fault_t non_crit_fault;
	} fault_index;
	char *diag;
} fault_data_t;

/**
 * @brief Put a fault in the fault queue.
 *
 * @param fault_data Pointer to struct containing data about the fault
 * @return osStatus_t Error code of osMessageQueuePut operation
 */
osStatus_t queue_fault(fault_data_t *fault_data);

/**
 * @brief Task for processing faults.
 *
 * @param pv_params NULL
 */
void vFaultHandler(void *pv_params);
extern osThreadId_t fault_handle;
extern const osThreadAttr_t fault_handle_attributes;
void clear_fault(void *args);
void process_fault(fault_data_t fault_data);
fault_sev_t get_max_severity();

#endif // FAULT_H
