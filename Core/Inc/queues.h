#ifndef CERBERUS_QUEUES_H
#define CERBERUS_QUEUES_H

#include "cmsis_os.h"
#include "cerberus_conf.h"


#define ONBOARD_TEMP_QUEUE_SIZE  8

typedef struct {
    uint16_t temperature;
    uint16_t humidity;
} onboard_temp_t;

extern osMessageQueueId_t onboard_temp_queue;


#endif // QUEUES_H