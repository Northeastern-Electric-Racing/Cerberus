#ifndef CERBERUS_FAULT_H
#define CERBERUS_FAULT_H

#include "cerberus_conf.h"
#include "cmsis_os.h"

typedef enum {
	DEFCON1 = 1,
	DEFCON2,
	DEFCON3,
	DEFCON4,
	DEFCON5,
	DEFCON_NONE
} fault_sev_t;

typedef enum {
	FAULTS_CLEAR = (1 << 0),
	ONBOARD_TEMP_FAULT = (1 << 1),
	ONBOARD_PEDAL_FAULT = (1 << 2),
	IMU_FAULT = (1 << 3),
	CAN_DISPATCH_FAULT = (1 << 4),
	CAN_ROUTING_FAULT = (1 << 5),
	FUSE_MONITOR_FAULT = (1 << 6),
	SHUTDOWN_MONITOR_FAULT = (1 << 7),
	DTI_ROUTING_FAULT = (1 << 8),
	STEERINGIO_ROUTING_FAULT = (1 << 9),
	STATE_RECEIVED_FAULT = (1 << 10),
	INVALID_TRANSITION_FAULT = (1 << 11),
	BMS_CAN_MONITOR_FAULT = (1 << 12),
	BUTTONS_MONITOR_FAULT = (1 << 13),
	BSPD_PREFAULT = (1 << 14),
	LV_MONITOR_FAULT = (1 << 15),
	RTDS_FAULT = (1 << 16),
	PDU_CURRENT_FAULT = ( 1 << 17),
	MAX_FAULTS = (1 << 18)
} fault_code_t;

typedef struct {
	fault_code_t id;
	fault_sev_t severity;
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
void clearFault(void *args);
fault_sev_t getMaxSeverity();

#endif // FAULT_H
