#ifndef CERBERUS_FAULT_H
#define CERBERUS_FAULT_H

#include "cmsis_os.h"
#include <stdbool.h>

typedef enum {
	ONBOARD_PEDAL_FAULT,
	CAN_DISPATCH_FAULT,
	CAN_ROUTING_FAULT,
	BMS_CAN_MONITOR_FAULT,
	MAX_CRITICAL_FAULT
} crit_fault_t;

// Unused Fault Message IDs
// DTI_ROUTING_FAULT
// STEERINGIO_ROUTING_FAULT
// STATE_RECEIVED_FAULT
// INVALID_TRANSITION_FAULT
// BUTTONS_MONITOR_FAULT

typedef enum {
	ONBOARD_TEMP_FAULT,
	IMU_FAULT,
	FUSE_MONITOR_FAULT,
	SHUTDOWN_MONITOR_FAULT,
	LV_MONITOR_FAULT,
	BSPD_PREFAULT,
	RTDS_FAULT,
	PUMP_SENSORS_FAULT,
	PDU_CURRENT_FAULT,
	MAX_NON_CRITICAL_FAULT
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

/**
 * @brief adds incoming fault data to current fault state
 * 
 * @param fault_data includes diag, index, and severity
 */
void process_fault(fault_data_t fault_data);

/**
 * @brief callback function to clear fault after timeout
 * 
 * @param args fault header with index and severity
 */
void clear_fault(void *args);

extern osThreadId_t fault_handle;
extern const osThreadAttr_t fault_handle_attributes;

#endif // FAULT_H
