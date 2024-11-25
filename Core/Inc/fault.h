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
	FAULTS_CLEAR = 0b01,
	ONBOARD_TEMP_FAULT = 0b010,
	ONBOARD_PEDAL_FAULT = 0b0100,
	IMU_FAULT = 0b01000,
	CAN_DISPATCH_FAULT = 0b010000,
	CAN_ROUTING_FAULT = 0b0100000,
	FUSE_MONITOR_FAULT = 0b01000000,
	SHUTDOWN_MONITOR_FAULT = 0b010000000,
	DTI_ROUTING_FAULT = 0b0100000000,
	STEERINGIO_ROUTING_FAULT = 0b01000000000,
	STATE_RECEIVED_FAULT = 0b010000000000,
	INVALID_TRANSITION_FAULT = 0b0100000000000,
	BMS_CAN_MONITOR_FAULT = 0b01000000000000,
	BUTTONS_MONITOR_FAULT = 0b010000000000000,
	BSPD_PREFAULT = 0b0100000000000000,
	LV_MONITOR_FAULT = 0b01000000000000000,
	RTDS_FAULT = 0b010000000000000000,
	MAX_FAULTS = 0b0100000000000000000
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
