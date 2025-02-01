#include "fault.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cerberus_conf.h"
#include "state_machine.h"

#define FAULT_HANDLE_QUEUE_SIZE 16
#define NUM_OF_FAULTS 18UL
#define SEND_FAULT_TIME 500 /* in millis */

osMessageQueueId_t fault_handle_high_priority_queue;
osMessageQueueId_t fault_handle_low_priority_queue;

uint32_t faults = 0;

osTimerId_t *timers = NULL;

fault_sev_t max_severity_level = DEFCON_NONE;
fault_sev_t *severity_levels = NULL;

osStatus_t queue_fault(fault_data_t *fault_data) {
  if (!fault_handle_high_priority_queue || !fault_handle_low_priority_queue)
    return osErrorParameter; // Return proper error code

  osStatus_t status; // Declare status once before the if/else block

  if (fault_data->severity <= DEFCON3) {
    status =
        osMessageQueuePut(fault_handle_high_priority_queue, fault_data, 0U, 0U);
  } else {
    status =
        osMessageQueuePut(fault_handle_low_priority_queue, fault_data, 0U, 0U);
  }

  return status;
}

osThreadId_t fault_handle;
const osThreadAttr_t fault_handle_attributes = {
    .name = "FaultHandler",
    .stack_size = 64 * 16,
    .priority = (osPriority_t)osPriorityRealtime7,
};

void vFaultHandler(void *pv_params) {
  // Create Timers Array
  if (timers == NULL) {
    timers = calloc(NUM_OF_FAULTS, sizeof(osTimerId_t));
  }

  // Create Severity Levels Array (all DEFCON_NONE initially)
  if (severity_levels == NULL) {
    severity_levels = calloc(NUM_OF_FAULTS, sizeof(fault_sev_t));
    for (int i = 0; i < NUM_OF_FAULTS; i++) {
      severity_levels[i] = DEFCON_NONE;
    }
  }

  fault_data_t fault_data;
  fault_handle_low_priority_queue =
      osMessageQueueNew(FAULT_HANDLE_QUEUE_SIZE, sizeof(fault_data_t), NULL);
  fault_handle_low_priority_queue =
      osMessageQueueNew(FAULT_HANDLE_QUEUE_SIZE, sizeof(fault_data_t), NULL);
  for (;;) {
    if (osMessageQueueGet(fault_handle_high_priority_queue, &fault_data, NULL,
                          pdMS_TO_TICKS(SEND_FAULT_TIME)) == osOK) {
      process_fault(fault_data);
    } else if (osMessageQueueGet(fault_handle_low_priority_queue, &fault_data,
                                 NULL,
                                 pdMS_TO_TICKS(SEND_FAULT_TIME)) == osOK) {
      process_fault(fault_data);
    }
  }
}

void process_fault(fault_data_t fault_data) {
  // Set Fault
  uint32_t *fault_id = malloc(sizeof(uint32_t));
  *fault_id = (uint32_t)fault_data.id;
  faults |= *fault_id;

  uint32_t index = (uint32_t)log2(*fault_id);

  // Create Timers
  if (!timers[index]) {
    timers[index] = osTimerNew(clear_fault, osTimerOnce, fault_id, NULL);
  }

  if (osTimerStart(timers[index], 4000) != osOK) {
    return;
  }

  // Get New Maximum Severity Level
  severity_levels[index] = fault_data.severity;
  max_severity_level = get_max_severity();

  switch (fault_data.severity) {
  case DEFCON1: /* Highest(1st) Priority */
    fault();
    break;
  case DEFCON2:
    fault();
    break;
  case DEFCON3:
    fault();
    break;
  case DEFCON4:
    break;
  case DEFCON5: /* Lowest Priority */
    break;
  case DEFCON_NONE:
    break;
  default:
    break;
  }

  // Send Can Message (even if a new fault was not received)
  can_msg_t msg;
  msg.id = CANID_FAULT_MSG;
  msg.len = 8;

  memcpy(msg.data, &faults, sizeof(faults));
  memcpy(msg.data + sizeof(faults), &max_severity_level,
         sizeof(max_severity_level));

  queue_can_msg(msg);
}

void clear_fault(void *args) {
  uint32_t *fault_id = (uint32_t *)args;

  // Remove this timer's fault from total faults
  faults &= ~(*fault_id);

  // Remove this timer's severity from total severity
  severity_levels[(uint32_t)log2(*fault_id)] = DEFCON_NONE;
  max_severity_level = get_max_severity();

  // unfault car if all critical faults are cleared
  if (max_severity_level > DEFCON3) {
    set_ready_mode();
  }

  free(fault_id);
}

fault_sev_t get_max_severity() {
  int max = DEFCON_NONE;
  for (int i = 0; i < NUM_OF_FAULTS; i++) {
    if (severity_levels[i] < max) {
      max = severity_levels[i];
    }
  }
  return (fault_sev_t)max;
}
