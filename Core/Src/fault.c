#include "fault.h"

osMessageQueueId_t fault_handle_queue;

//Function to determine priority base on severity
void priority(void) {
    osThreadId_t id;
    MSGQUEUE_OBJ_t msg;
    osStatus_t status;
    // Wait until a message is in the queue, send messages when they are in the queue
	for(;;) {
        status = osMessageQueueGet(fault_handle_queue, &msg, NULL, 0U);   // wait for message
        id = osThreadGetId(); // Obtain ID of current running thread
        if (status == osOK) {
        
        // process data
        switch (fault_sev_t) {
        case 'DEFCON1': //Higest(1st) Priority
            osThreadSetPriority(id, osPriorityHigh);
            osThreadSetPriority(id, osPriorityHigh);
            break;
        case 'DEFCON2': //2nd Highest Priority
            osThreadSetPriority(id, osPriorityAboveNormal);
            osThreadSetPriority(id, osPriorityAboveNormal);
            break;
        case 'DEFCON3': //3rd Highest Priority
            osThreadSetPriority(id, osPriorityNormal);
            osThreadSetPriority(id, osPriorityNormal);
            break;
        case 'DEFCON4': //Slight Above Lowest Priority
            osThreadSetPriority(id, osPriorityBelowNormal);
            osThreadSetPriority(id, osPriorityBelowNormal);
            break;
        case 'DEFCON5': //Lowest Priority
            osThreadSetPriority(id, osPriorityLow);
            osThreadSetPriority(id, osPriorityLow);
            break;
        default: //Unable To Identify 'fault_sev_t'
            osThreadSetPriority(id, osPriorityIdle);
            osThreadSetPriority(id, osPriorityIdle);
            break;
            }
        }
    }
}