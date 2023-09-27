#include <sht30.h>
#include <FreeRTOS.h>
#include "cerberus_conf.h"
#include "task.h"
#include "can.h"

void vTempMonitor(void *pv_params)
{
    static const uint8_t num_samples = 10;

    sht30_t temp_sensor;
    I2C_HandleTypeDef *hi2c1;
    can_msg_t temp_msg = {
        .id = CANID_TEMP_SENSOR,
        .len = 4, /* bytes */
        .line = CAN_LINE_1
    };
    struct {
        uint16_t temperature;
        uint16_t humidity;
    } sensor_data;
    

    hi2c1 = (I2C_HandleTypeDef *)pv_params;

    if (sht30_init(&temp_sensor, &hi2c1)) {
        //TODO: Handle error
    }

    for(;;) {

        /* Take measurement */
        if (sht30_get_temp_humid(&temp_sensor)) {
            //TODO: Handler error
        }

        /* Run values through LPF of sample size  */
        sensor_data.temperature = (sensor_data.temperature + temp_sensor.temp) 
                                  / num_samples;
        sensor_data.humidity = (sensor_data.humidity + temp_sensor.humidity) 
                               / num_samples;

        /* Send CAN message */
        temp_msg.data = (uint8_t *) sensor_data;
        if (can_send_message(temp_msg)) {
            //TODO: Handle error
        }

        /* Yield to other tasks */
        taskYIELD();
    }
}