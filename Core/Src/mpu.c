#include "mpu.h"
#include "stm32f405xx.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define YLED_PIN      GPIO_PIN_8
#define RLED_PIN      GPIO_PIN_9
#define WATCHDOG_PIN  GPIO_PIN_15
#define CAN_FAULT_PIN GPIO_PIN_3

#define ADC_TIMEOUT 2 /* ms */

static osMutexAttr_t mpu_i2c_mutex_attr;
static osMutexAttr_t mpu_adc_mutex_attr;

mpu_t *init_mpu(I2C_HandleTypeDef *hi2c, ADC_HandleTypeDef *pedals_adc,
		ADC_HandleTypeDef *lv_adc, GPIO_TypeDef *led_gpio,
		GPIO_TypeDef *watchdog_gpio)
{
	assert(hi2c);
	assert(pedals_adc);
	assert(lv_adc);
	assert(led_gpio);
	assert(watchdog_gpio);

	/* Create MPU struct */
	mpu_t *mpu = malloc(sizeof(mpu_t));
	assert(mpu);

	mpu->hi2c = hi2c;
	mpu->pedals_adc = pedals_adc;
	mpu->lv_adc = lv_adc;
	mpu->led_gpio = led_gpio;
	mpu->watchdog_gpio = watchdog_gpio;

    /* Initialize the Onboard Temperature Sensor */
    // CHANGE: Updated SHT30 initialization
    mpu->temp_sensor.i2c_handle = hi2c;
    mpu->temp_sensor.write = (Write_ptr)HAL_I2C_Master_Transmit;
    mpu->temp_sensor.read = (Read_Ptr)HAL_I2C_Master_Receive;
    mpu->temp_sensor.mem_read = (Mem_Read_Ptr)HAL_I2C_Mem_Read;
    mpu->temp_sensor.delay = HAL_Delay;
    assert(sht30_init(&mpu->temp_sensor) == 0);
	assert(!HAL_ADC_Start_DMA(mpu->pedals_adc, mpu->pedal_dma_buf,
				  sizeof(mpu->pedal_dma_buf) /
					  sizeof(uint32_t)));

	assert(!HAL_ADC_Start_DMA(mpu->lv_adc, &mpu->lv_dma_buf,
				  sizeof(mpu->lv_dma_buf) / sizeof(uint32_t)));

	/* Initialize the IMU */
	mpu->imu = malloc(sizeof(lsm6dso_t));
	assert(mpu->imu);
	//assert(!lsm6dso_init(mpu->imu, mpu->hi2c)); /* This is always connected */

	/* Create Mutexes */
	mpu->i2c_mutex = osMutexNew(&mpu_i2c_mutex_attr);
	assert(mpu->i2c_mutex);

	mpu->adc_mutex = osMutexNew(&mpu_adc_mutex_attr);
	assert(mpu->adc_mutex);

	HAL_GPIO_WritePin(mpu->led_gpio, CAN_FAULT_PIN, GPIO_PIN_SET);

	return mpu;
}

int8_t write_rled(mpu_t *mpu, bool status)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(mpu->led_gpio, RLED_PIN, status);
	return 0;
}

int8_t toggle_rled(mpu_t *mpu)
{
	if (!mpu)
		return -1;

	HAL_GPIO_TogglePin(mpu->led_gpio, RLED_PIN);
	return 0;
}

int8_t write_yled(mpu_t *mpu, bool status)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(mpu->led_gpio, YLED_PIN, status);
	return 0;
}

int8_t toggle_yled(mpu_t *mpu)
{
	if (!mpu)
		return -1;

	HAL_GPIO_TogglePin(mpu->led_gpio, YLED_PIN);
	return 0;
}

int8_t pet_watchdog(mpu_t *mpu)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(mpu->watchdog_gpio, WATCHDOG_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(mpu->watchdog_gpio, WATCHDOG_PIN, GPIO_PIN_RESET);
	return 0;
}

void read_lv_voltage(mpu_t *mpu, uint32_t *lv_buf)
{
	memcpy(lv_buf, &mpu->lv_dma_buf, sizeof(mpu->lv_dma_buf));
}

void read_pedals(mpu_t *mpu, uint32_t pedal_buf[4])
{
	memcpy(pedal_buf, mpu->pedal_dma_buf, sizeof(mpu->pedal_dma_buf));
}

int8_t read_temp_sensor(mpu_t *mpu, uint16_t *temp, uint16_t *humidity)
{
	if (!mpu)
		return -1;

	osStatus_t mut_stat = osMutexAcquire(mpu->i2c_mutex, osWaitForever);
	if (mut_stat)
		return mut_stat;

    // updated to use int return type
    int status = sht30_get_temp_humid(&mpu->temp_sensor);
    if (status != 0)
        return status;s

	*temp = mpu->temp_sensor.temp;
	*humidity = mpu->temp_sensor.humidity;

	osMutexRelease(mpu->i2c_mutex);
	return 0;
}

int8_t read_accel(mpu_t *mpu, uint16_t accel[3])
{
	if (!mpu)
		return -1;

	osStatus_t mut_stat = osMutexAcquire(mpu->i2c_mutex, osWaitForever);
	if (mut_stat)
		return mut_stat;

    HAL_StatusTypeDef hal_stat = lsm6dso_read_accel(mpu->imu);
    if (hal_stat != HAL_OK)
        return hal_stat;
	memcpy(accel, mpu->imu->accel_data, 3);

	osMutexRelease(mpu->i2c_mutex);
	return 0;
}

int8_t read_gyro(mpu_t *mpu, uint16_t gyro[3])
{
	if (!mpu)
		return -1;

	osStatus_t mut_stat = osMutexAcquire(mpu->i2c_mutex, osWaitForever);
	if (mut_stat)
		return mut_stat;

	HAL_StatusTypeDef hal_stat = lsm6dso_read_gyro(mpu->imu);
	if (hal_stat)
		return hal_stat;

	memcpy(gyro, mpu->imu->gyro_data, 3);

	osMutexRelease(mpu->i2c_mutex);
	return 0;
}
