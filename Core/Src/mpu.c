#include "mpu.h"
#include "stm32f405xx.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define YLED_PIN	 GPIO_PIN_8
#define RLED_PIN	 GPIO_PIN_9
#define WATCHDOG_PIN GPIO_PIN_15
#define ADC_TIMEOUT	 2 /* ms */

static osMutexAttr_t mpu_i2c_mutex_attr;
static osMutexAttr_t mpu_adc_mutex_attr;

mpu_t* init_mpu(I2C_HandleTypeDef* hi2c, ADC_HandleTypeDef* accel_adc1,
				ADC_HandleTypeDef* accel_adc2, ADC_HandleTypeDef* brake_adc, 
                GPIO_TypeDef* led_gpio, GPIO_TypeDef* watchdog_gpio)
{
	assert(hi2c);
	assert(accel_adc1);
	assert(accel_adc2);
	assert(brake_adc);
	assert(led_gpio);
	assert(watchdog_gpio);

	/* Create MPU struct */
	mpu_t* mpu = malloc(sizeof(mpu_t));
	assert(mpu);

	mpu->hi2c		   = hi2c;
	mpu->accel_adc1	   = accel_adc1;
	mpu->accel_adc2	   = accel_adc2;
	mpu->brake_adc	   = brake_adc;
	mpu->led_gpio	   = led_gpio;
	mpu->watchdog_gpio = watchdog_gpio;

	/* Initialize the Onboard Temperature Sensor */
	mpu->temp_sensor = malloc(sizeof(sht30_t));
	assert(mpu->temp_sensor);
	mpu->temp_sensor->i2c_handle = hi2c;
	//assert(!sht30_init(mpu->temp_sensor)); /* This is always connected */

	/* Initialize the IMU */
	mpu->imu = malloc(sizeof(lsm6dso_t));
	assert(mpu->imu);
	
	//assert(!lsm6dso_init(mpu->imu, mpu->hi2c)); /* This is always connected */

	/* Create Mutexes */
	mpu->i2c_mutex = osMutexNew(&mpu_i2c_mutex_attr);
	assert(mpu->i2c_mutex);

	mpu->adc_mutex = osMutexNew(&mpu_adc_mutex_attr);
	assert(mpu->adc_mutex);

	return mpu;
}

int8_t write_rled(mpu_t* mpu, bool status)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(mpu->led_gpio, RLED_PIN, status);
	return 0;
}

int8_t toggle_rled(mpu_t* mpu)
{
	if (!mpu)
		return -1;

	HAL_GPIO_TogglePin(mpu->led_gpio, RLED_PIN);
	return 0;
}

int8_t write_yled(mpu_t* mpu, bool status)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(mpu->led_gpio, YLED_PIN, status);
	return 0;
}

int8_t toggle_yled(mpu_t* mpu)
{
	if (!mpu)
		return -1;

	HAL_GPIO_TogglePin(mpu->led_gpio, YLED_PIN);
	return 0;
}

int8_t pet_watchdog(mpu_t* mpu)
{
	if (!mpu)
		return -1;

	HAL_GPIO_WritePin(mpu->watchdog_gpio, WATCHDOG_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(mpu->watchdog_gpio, WATCHDOG_PIN, GPIO_PIN_RESET);
	return 0;
}

static int8_t start_adcs(mpu_t* mpu)
{
	HAL_StatusTypeDef hal_stat;

	hal_stat = HAL_ADC_Start(mpu->accel_adc1);
	if (hal_stat)
		return hal_stat;

	hal_stat = HAL_ADC_Start(mpu->accel_adc2);
	if (hal_stat)
		return hal_stat;

	hal_stat = HAL_ADC_Start(mpu->brake_adc);
	if (hal_stat)
		return hal_stat;

	return 0;
}

static int8_t poll_adc_threaded(ADC_HandleTypeDef* adc)
{
	HAL_StatusTypeDef hal_stat = HAL_TIMEOUT;
	while (hal_stat == HAL_TIMEOUT) {
		hal_stat = HAL_ADC_PollForConversion(adc, ADC_TIMEOUT);

		if (hal_stat == HAL_TIMEOUT)
			osThreadYield();
	}
	return hal_stat;
}

/* Note: this should be called from within a thread since it yields to scheduler */
int8_t read_adc(mpu_t* mpu, uint16_t raw[3])
{
	if (!mpu)
		return -1;

	osStatus_t mut_stat = osMutexAcquire(mpu->adc_mutex, osWaitForever);
	if (mut_stat)
		return mut_stat;

	HAL_StatusTypeDef hal_stat = start_adcs(mpu);
	if (hal_stat)
		return hal_stat;

	hal_stat = poll_adc_threaded(mpu->accel_adc1);
	if (hal_stat)
		return hal_stat;

	hal_stat = poll_adc_threaded(mpu->accel_adc2);
	if (hal_stat)
		return hal_stat;

	hal_stat = poll_adc_threaded(mpu->brake_adc);
	if (hal_stat)
		return hal_stat;

	raw[0] = HAL_ADC_GetValue(mpu->accel_adc1);
	raw[1] = HAL_ADC_GetValue(mpu->accel_adc2);
	raw[2] = HAL_ADC_GetValue(mpu->brake_adc);

	osMutexRelease(mpu->adc_mutex);

	return 0;
}

int8_t read_temp_sensor(mpu_t* mpu, uint16_t* temp, uint16_t* humidity)
{
	if (!mpu)
		return -1;

	osStatus_t mut_stat = osMutexAcquire(mpu->i2c_mutex, osWaitForever);
	if (mut_stat)
		return mut_stat;

	HAL_StatusTypeDef hal_stat = sht30_get_temp_humid(mpu->temp_sensor);
	if (hal_stat)
		return hal_stat;

	*temp	  = mpu->temp_sensor->temp;
	*humidity = mpu->temp_sensor->humidity;

	osMutexRelease(mpu->i2c_mutex);
	return 0;
}

int8_t read_accel(mpu_t* mpu, uint16_t accel[3])
{
	if (!mpu)
		return -1;

	osStatus_t mut_stat = osMutexAcquire(mpu->i2c_mutex, osWaitForever);
	if (mut_stat)
		return mut_stat;

	HAL_StatusTypeDef hal_stat = lsm6dso_read_accel(mpu->imu);
	if (hal_stat)
		return hal_stat;

	memcpy(accel, mpu->imu->accel_data, 3);

	osMutexRelease(mpu->i2c_mutex);
	return 0;
}

int8_t read_gyro(mpu_t* mpu, uint16_t gyro[3])
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
