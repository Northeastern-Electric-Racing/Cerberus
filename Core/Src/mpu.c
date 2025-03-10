#include "mpu.h"
#include "main.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_utils.h"

#define ADC_TIMEOUT 2 /* ms */

static osMutexAttr_t mpu_i2c_mutex_attr;
static osMutexAttr_t mpu_adc_mutex_attr;
extern I2C_HandleTypeDef hi2c1; /* defined in main.c */
// static inline int read_reg(uint8_t *data, uint8_t reg, uint8_t length)
// {
// 	return HAL_I2C_Mem_Read(hi2c, LSM6DSO_I2C_ADDRESS, reg,
// 				I2C_MEMADD_SIZE_8BIT, data, length,
// 				HAL_MAX_DELAY);
// }

// static inline int write_reg(uint8_t *data, uint8_t reg, uint8_t length)
// {
// 	return HAL_I2C_Mem_Write(hi2c, LSM6DSO_I2C_ADDRESS, reg,
// 				 I2C_MEMADD_SIZE_8BIT, data, length,
// 				 HAL_MAX_DELAY);
// }

mpu_t *init_mpu(ADC_HandleTypeDef *pedals_adc, ADC_HandleTypeDef *lv_adc)
{
	assert(pedals_adc);
	assert(lv_adc);

	/* Create MPU struct */
	mpu_t *mpu = malloc(sizeof(mpu_t));
	assert(mpu);

	mpu->hi2c = &hi2c1;
	mpu->pedals_adc = pedals_adc;
	mpu->lv_adc = lv_adc;

	/* Initialize the Onboard Temperature Sensor */
	// mpu->temp_sensor = malloc(sizeof(sht30_t));
	// assert(mpu->temp_sensor);
	// mpu->temp_sensor->i2c_handle = hi2c;
	// assert(!sht30_init(mpu->temp_sensor)); /* This is always connected */

	assert(!HAL_ADC_Start_DMA(mpu->pedals_adc, mpu->pedal_dma_buf,
				  sizeof(mpu->pedal_dma_buf) /
					  sizeof(uint32_t)));

	assert(!HAL_ADC_Start_DMA(mpu->lv_adc, &mpu->lv_dma_buf,
				  sizeof(mpu->lv_dma_buf) / sizeof(uint32_t)));

	/* Initialize the IMU */
	// mpu->imu = malloc(sizeof(lsm6dso_t));
	// assert(mpu->imu);
	// assert(!lsm6dso_init(mpu->imu, read_reg,
	// 		     write_reg)); /* This is always connected */

	/* Create Mutexes */
	mpu->i2c_mutex = osMutexNew(&mpu_i2c_mutex_attr);
	assert(mpu->i2c_mutex);

	mpu->adc_mutex = osMutexNew(&mpu_adc_mutex_attr);
	assert(mpu->adc_mutex);

	return mpu;
}

int8_t write_rled(mpu_t *mpu, bool status)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(DEBUG_LED2_GPIO_Port, DEBUG_LED2_Pin, status);
	return 0;
}

int8_t toggle_rled(mpu_t *mpu)
{
	if (!mpu)
		return -1;

	HAL_GPIO_TogglePin(DEBUG_LED2_GPIO_Port, DEBUG_LED2_Pin);
	return 0;
}

int8_t write_yled(mpu_t *mpu, bool status)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(DEBUG_LED1_GPIO_Port, DEBUG_LED1_Pin, status);
	return 0;
}

int8_t toggle_yled(mpu_t *mpu)
{
	if (!mpu)
		return -1;

	HAL_GPIO_TogglePin(DEBUG_LED1_GPIO_Port, DEBUG_LED1_Pin);
	return 0;
}

int8_t pet_watchdog(mpu_t *mpu)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(WATCHDOG_GPIO_Port, WATCHDOG_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(WATCHDOG_GPIO_Port, WATCHDOG_Pin, GPIO_PIN_RESET);
	return 0;
}

void read_lv_voltage(mpu_t *mpu, uint16_t *lv_buf)
{
	memcpy(lv_buf, &mpu->lv_dma_buf, sizeof(*lv_buf));
}

void read_pedals(mpu_t *mpu, uint32_t pedal_buf[4])
{
	memcpy(pedal_buf, mpu->pedal_dma_buf, sizeof(mpu->pedal_dma_buf));
}

// int8_t read_temp_sensor(mpu_t *mpu, uint16_t *temp, uint16_t *humidity)
// {
// 	if (!mpu)
// 		return -1;

// 	osStatus_t mut_stat = osMutexAcquire(mpu->i2c_mutex, osWaitForever);
// 	if (mut_stat)
// 		return mut_stat;

// 	HAL_StatusTypeDef hal_stat = sht30_get_temp_humid(mpu->temp_sensor);
// 	if (hal_stat)
// 		return hal_stat;

// 	*temp = mpu->temp_sensor->temp;
// 	*humidity = mpu->temp_sensor->humidity;

// 	osMutexRelease(mpu->i2c_mutex);
// 	return 0;
// }

// int8_t read_accel(mpu_t *mpu)
// {
// 	if (!mpu)
// 		return -1;

// 	osStatus_t mut_stat = osMutexAcquire(mpu->i2c_mutex, osWaitForever);
// 	if (mut_stat)
// 		return mut_stat;

// 	HAL_StatusTypeDef hal_stat = lsm6dso_read_accel(mpu->imu);
// 	if (hal_stat)
// 		return hal_stat;

// 	osMutexRelease(mpu->i2c_mutex);
// 	return 0;
// }

// int8_t read_gyro(mpu_t *mpu)
// {
// 	if (!mpu)
// 		return -1;

// 	osStatus_t mut_stat = osMutexAcquire(mpu->i2c_mutex, osWaitForever);
// 	if (mut_stat)
// 		return mut_stat;

// 	HAL_StatusTypeDef hal_stat = lsm6dso_read_gyro(mpu->imu);
// 	if (hal_stat)
// 		return hal_stat;

// 	osMutexRelease(mpu->i2c_mutex);
// 	return 0;
// }

/**
 * @brief Write the MPU FAULT line to the car
 * 
 * @param mpu 
 * @param status true (faulted) or false (unfaulted)
 * @return int8_t, -1 if failure, 0 if success
 */
int8_t write_fault(mpu_t *mpu, bool status)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(MCU_FAULT_GPIO_Port, MCU_FAULT_Pin, !status);

	return 0;
}
