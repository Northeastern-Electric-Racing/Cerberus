#ifndef MPU_H
#define MPU_H

#include "cmsis_os.h"
#include "lsm6dso.h"
#include "sht30.h"
#include "stm32f405xx.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	I2C_HandleTypeDef *hi2c;
	ADC_HandleTypeDef *pedals_adc;
	uint32_t pedal_dma_buf[4];

	ADC_HandleTypeDef *lv_adc;
	uint32_t lv_dma_buf;

	GPIO_TypeDef *led_gpio;
	GPIO_TypeDef *watchdog_gpio;
	sht30_t temp_sensor;
	lsm6dso_t *imu;
	osMutexId_t *adc_mutex;
	osMutexId_t *i2c_mutex;
	/* Not including LED Mutexes because not necessary */
} mpu_t;

typedef struct {
	int8_t channel_0;
	int8_t channel_1;
} brake_adc_channels_t;

/**
 * @brief Initialize MPU devices.
 * 
 * @param hi2c Pointer to struct representing i2c1
 * @param pedals_adc Pointer to struct representing pedals ADC
 * @param lv_adc Pointer to struct representing LV battery ADC
 * @param led_gpio Pointer to struct represneitng LED GPIO
 * @param watchdog_gpio Pointer to struct represneting watchdog GPIO
 * @return mpu_t* Pointer to struct representing the MPU
 */
mpu_t *init_mpu(I2C_HandleTypeDef *hi2c, ADC_HandleTypeDef *pedals_adc,
		ADC_HandleTypeDef *lv_adc, GPIO_TypeDef *led_gpio,
		GPIO_TypeDef *watchdog_gpio);

/**
 * @brief Read pedal ADCs with DMA.
 * 
 * @param mpu Pointer to struct representing MPU
 * @param pedal_buf Buffer that pedal data will be written to
 */
void read_pedals(mpu_t *mpu, uint32_t pedal_buf[4]);

/**
 * @brief Read the voltage of the low voltage batteries.
 * 
 * @param mpu Pointer to struct representing the MPU.
 * @param lv_buf Pointer to location where raw ADC value will be stored.
 */
void read_lv_voltage(mpu_t *mpu, uint32_t *lv_buf);

/* Unused ----------------------------------------------------------------- */

int8_t write_rled(mpu_t *mpu, bool status);

int8_t toggle_rled(mpu_t *mpu);

int8_t write_yled(mpu_t *mpu, bool status);

int8_t toggle_yled(mpu_t *mpu);

int8_t pet_watchdog(mpu_t *mpu);

int8_t read_temp_sensor(mpu_t *mpu, uint16_t *temp, uint16_t *humidity);

int8_t read_accel(mpu_t *mpu, uint16_t accel[3]);

int8_t read_gyro(mpu_t *mpu, uint16_t gyro[3]);

#endif /* MPU */