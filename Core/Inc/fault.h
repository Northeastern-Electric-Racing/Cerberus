#ifndef CERBERUS_FAULT_H
#define CERBERUS_FAULT_H

#include "cerberus_conf.h"
#include "cmsis_os.h"

typedef enum { DEFCON1 = 1, DEFCON2, DEFCON3, DEFCON4, DEFCON5 } fault_sev_t;

typedef enum {
	FAULTS_CLEAR = 0x0,
	ONBOARD_TEMP_FAULT = 0x1,
	ONBOARD_PEDAL_FAULT = 0x2,
	IMU_FAULT = 0x4,
	CAN_DISPATCH_FAULT = 0x8,
	CAN_ROUTING_FAULT = 0x10,
	FUSE_MONITOR_FAULT = 0x20,
	SHUTDOWN_MONITOR_FAULT = 0x40,
	DTI_ROUTING_FAULT = 0x80,
	STEERINGIO_ROUTING_FAULT = 0x100,
	STATE_RECEIVED_FAULT = 0x200,
	INVALID_TRANSITION_FAULT = 0x400,
	BMS_CAN_MONITOR_FAULT = 0x800,
	BUTTONS_MONITOR_FAULT = 0xF00,
	BSPD_PREFAULT = 0x1000,
	LV_MONITOR_FAULT = 0x2000,
	RTDS_FAULT = 0x4000,
	MAX_FAULTS
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

#endif // FAULT_H
