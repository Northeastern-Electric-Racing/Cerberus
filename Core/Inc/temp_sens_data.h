#ifndef TEMP_SENS_DATA_H
#define TEMP_SENS_DATA_H

#include "cmsis_os.h"

/* Defining Temperature Sensor Data Collection Task */
void vTempSensorData(void *pv_params);
// Took this from monitor.h done by Nick Depatie as it seems
// our tasks are simlar
osThreadId_t temp_sensor_data_handle;
const osThreadAttr_t temp_sensor_data_attributes = {
	.name = "TempSensorData",
	.stack_size = 128 * 4,
    // ASK Hamza about priority level
	.priority = (osPriority_t) osPriorityBelowNormal3,
};


#endif //TEMP_SENS_DATA_H
