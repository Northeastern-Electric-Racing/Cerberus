#include "fault.h"

osMessageQueueId_t fault_handle_queue;

osThreadId_t fault_handle;
const osThreadAttr_t fault_handle_attributes = {
	.name		= "FaultHandler",
	.stack_size = 128 * 4,
	.priority	= (osPriority_t)osPriorityISR,
};

//Function assign a priority and tell the kernel to schedule on boot
void vFaultHandler(void* pv_params) {
    fault_data_t fault_data;
    osStatus_t status;
    //Wait until a message is in the queue, send messages when they are in the queue
	for(;;) {
        status = osMessageQueueGet(fault_handle_queue, &fault_data, NULL, 0U);   // wait for message
        if (status == osOK) {
        
        // process data
        switch (fault_data.severity) {
        case 'DEFCON1': //Higest(1st) Priority
            break;
        case 'DEFCON2': //2nd Highest Priority
            break;
        case 'DEFCON3': //3rd Highest Priority
            break;
        case 'DEFCON4': //Slight Above Lowest Priority
            break;
        case 'DEFCON5': //Lowest Priority
            break;
        default: //Unable To Identify 'fault_sev_t'
            break;
            }
        }
    }
}