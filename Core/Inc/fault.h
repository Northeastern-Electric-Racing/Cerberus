#ifndef CERBERUS_FAULT_H
#define CERBERUS_FAULT_H

#include "cmsis_os.h"
#include "cerberus_conf.h"

#define FAULT_HANDLE_QUEUE_SIZE 16

typedef enum {
    DEFCON1 = 1,
    DEFCON2,
    DEFCON3,
    DEFCON4,
    DEFCON5
} fault_sev_t;

typedef enum {
    FAULTS_CLEAR        = 0x0,
    ONBOARD_TEMP_FAULT  = 0x1,
    ONBOARD_PEDAL_FAULT = 0x2,
    IMU_FAULT           = 0x4,
} fault_code_t;

//TODO: Make this queue not accessible to users,
//  Make this into a wrapped function for "trigger_fault"
//  and then expose _that_ to the user
typedef struct {
    fault_code_t id;
    fault_sev_t severity;
    char *diag;
} fault_data_t;

//TODO: Make this queue not accessible to users,
//  Make this into a wrapped function for "trigger_fault"
//  and then expose _that_ to the user
extern osMessageQueueId_t fault_handle_queue;

/* Defining Fault Hanlder Task */
void vFaultHandler(void *pv_params);
extern osThreadId_t fault_handle;
const osThreadAttr_t fault_handle_attributes;

#endif // FAULT_H
