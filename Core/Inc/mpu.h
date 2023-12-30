#ifndef MPU_H
#define MPU_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f405xx.h"
#include "sht30.h"
#include "lsm6dso.h"
#include "cmsis_os.h"

typedef struct {
    I2C_HandleTypeDef *hi2c;
    ADC_HandleTypeDef *accel_adc1;
    ADC_HandleTypeDef *accel_adc2;
    ADC_HandleTypeDef *brake_adc;
    GPIO_TypeDef* led_gpio;
    GPIO_TypeDef* watchdog_gpio;
    sht30_t *temp_sensor;
    lsm6dso_t *imu;
    osMutexId_t *adc_mutex;
    osMutexId_t *i2c_mutex;
    /* Not including LED Mutexes because not necessary */
} mpu_t;

mpu_t *init_mpu(I2C_HandleTypeDef *hi2c, ADC_HandleTypeDef *accel_adc1, 
                ADC_HandleTypeDef *accel_adc2, ADC_HandleTypeDef *brake_adc,
                GPIO_TypeDef* led_gpio, GPIO_TypeDef* watchdog_gpio);

int8_t write_rled(mpu_t *mpu, bool status);

int8_t toggle_rled(mpu_t *mpu);

int8_t write_yled(mpu_t *mpu, bool status);

int8_t toggle_yled(mpu_t *mpu);

/* Note: this should be called from within a thread since it yields to scheduler */
int8_t read_adc(mpu_t *mpu, uint16_t raw[3]);

int8_t read_temp_sensor(mpu_t *mpu, uint16_t *temp, uint16_t *humidity);

int8_t read_accel(mpu_t *mpu, uint16_t accel[3]);

int8_t read_gyro(mpu_t *mpu, uint16_t gyro[3]);

#endif /* MPU */