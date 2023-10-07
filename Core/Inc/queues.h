#ifndef QUEUES_H
#define QUEUES_H

#include "cmsis_os.h"


#define NUM_ONBOARD_TEMP_QUEUE  8

typedef struct {
    uint16_t temperature;
    uint16_t humidity;
} onboard_temp_t;

extern osMessageQueueId_t onboard_temp_queue;

#endif // QUEUES_H
