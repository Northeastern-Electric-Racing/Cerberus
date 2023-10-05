#include <sht30.h>
#include <FreeRTOS.h>
#include "task.h"

void vTempSensorData(void *pv_params)
{
    /* Task Delay */
    static const uint8_t temp_sensor_delay = 500; /* ms */

    sht30_t temp_sensor;
    I2C_HandleTypeDef *hi2c1;

    /* Struct of temperature data */
    struct {
        uint16_t temperature;
    } sensor_data;

    hi2c1 = (I2C_HandleTypeDef *)pv_params;
    /* Checks if struct sht30 initialized properly */
    if (sht30_init(&temp_sensor, &hi2c1)) {
		//How to handle if there is an error?
	}

    
    for(;;)
    {
        /* Get Temperature Measurement*/
        if (sht30_get_temp_humid(&temp_sensor)) {
			//TODO: Handler error
		}

        /* Grabs the current temperature at the time the task is run */
        sensor_data.temperature = (temp_sensor.temp);
    }

    /* Task Delay */
    osDelayUntil(temp_sensor_delay);
}