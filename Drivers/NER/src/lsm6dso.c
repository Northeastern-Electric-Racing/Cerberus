/*
	LSM6DSOXTR IMU DRIVER Source File
	Link to part Datasheet for reference:
	https://www.st.com/resource/en/datasheet/lsm6dsox.pdf

	Author: Hamza Iqbal
*/

#include "lsm6dso.h"

#define REG_RESOLUTION                  32768   /* Half the range of a 16 bit signed integer */
#define ACCEL_RANGE                     4       /* The range of values in g's returned from accelerometer */
#define GYRO_RANGE                      1000    /* The range of values from the gyro in dps */

static inline HAL_StatusTypeDef lsm6dso_read_reg(lsm6dso_t *imu, uint8_t *data, uint8_t reg)
{
	return HAL_I2C_Mem_Read(imu->i2c_handle, LSM6DSO_I2C_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY);
}

static inline HAL_StatusTypeDef lsm6dso_read_mult_reg(lsm6dso_t *imu, uint8_t *data, uint8_t reg, uint8_t length)
{
	return HAL_I2C_Mem_Read(imu->i2c_handle, LSM6DSO_I2C_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, data, length, HAL_MAX_DELAY);
}

static inline HAL_StatusTypeDef lsm6dso_write_reg(lsm6dso_t *imu, uint8_t reg, uint8_t *data)
{
	return HAL_I2C_Mem_Write(imu->i2c_handle, LSM6DSO_I2C_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY);
}

static int16_t accel_data_convert(raw_accel)
{
	int8_t msb, lsb;
	int16_t val;
	msb = (raw_accel & 0x00FF) << 8;
	lsb = (raw_accel & 0xFF00) >> 8;
	val = msb | lsb;

	return (int16_t)(((int32_t)val * ACCEL_RANGE * 1000) / REG_RESOLUTION);
}

static int16_t gyro_data_convert(gyro_accel)
{
	int8_t msb, lsb;
	int16_t val;
	msb = (gyro_accel & 0x00FF) << 8;
	lsb = (gyro_accel & 0xFF00) >> 8;
	val = msb | lsb;
	return (int16_t)(((int32_t)val * GYRO_RANGE * 100) / REG_RESOLUTION);
}

static HAL_StatusTypeDef lsm6dso_ping_imu(lsm6dso_t *imu)
{
	uint8_t reg_data;
	HAL_StatusTypeDef status;

	status = lsm6dso_read_reg(imu, &reg_data, LSM6DSO_REG_DEVICE_ID);
	if (status != HAL_OK)
		return status;

	if(reg_data != 0x6C)
		return HAL_ERROR;

	return HAL_OK;
}

HAL_StatusTypeDef lsm6dso_set_accel_cfg(lsm6dso_t *imu, int8_t odr_sel, int8_t fs_sel, int8_t lp_f2_enable)
{
	uint8_t config = (((odr_sel << 4) | (fs_sel << 2) | (lp_f2_enable << 1)) << 1);
	imu->accel_config = config;

	return lsm6dso_write_reg(imu, LSM6DSO_REG_ACCEL_CTRL, &imu->accel_config);
}

HAL_StatusTypeDef lsm6dso_gyro_cfg(lsm6dso_t *imu, int8_t odr_sel, int8_t fs_sel, int8_t fs_125)
{
	uint8_t config = (((odr_sel << 4) | (fs_sel << 2) | (fs_125 << 1)) << 1);
	imu->gyro_config = config;

	return lsm6dso_write_reg(imu, LSM6DSO_REG_GYRO_CTRL, &imu->gyro_config);
}

HAL_StatusTypeDef lsm6dso_init(lsm6dso_t *imu, I2C_HandleTypeDef *i2c_handle)
{
	HAL_StatusTypeDef status;

	imu->i2c_handle = i2c_handle;

	imu->accel_data[LSM6DSO_X_AXIS] = 0;
	imu->accel_data[LSM6DSO_Y_AXIS] = 0;
	imu->accel_data[LSM6DSO_Z_AXIS] = 0;

	imu->gyro_data[LSM6DSO_X_AXIS] = 0;
	imu->gyro_data[LSM6DSO_Y_AXIS] = 0;
	imu->gyro_data[LSM6DSO_Z_AXIS] = 0;

	/* Quick check to make sure I2C is working */
	status = lsm6dso_ping_imu(imu);
	if(status != HAL_OK)
		return status;

	/*
		Configure IMU to default params
		Refer to datasheet pages 56-57
	*/
	status = lsm6dso_set_accel_cfg(imu, 0x08, 0x02, 0x01);
	if (status != HAL_OK)
		return status;

	status = lsm6dso_gyro_cfg(imu, 0x08, 0x02, 0x00);
	if (status != HAL_OK)
		return status;

	return HAL_OK;
}

HAL_StatusTypeDef lsm6dso_read_accel(lsm6dso_t *imu)
{
	union {
		uint8_t buf[2];
		int16_t data;
	} accel_x_raw, accel_y_raw, accel_z_raw;
	HAL_StatusTypeDef status;

	/* Getting raw data from registers */
	status = lsm6dso_read_mult_reg(imu, accel_x_raw.buf, LSM6DSO_REG_ACCEL_X_AXIS_L, 2);
	if(status != HAL_OK)
		return status;

	status = lsm6dso_read_mult_reg(imu, accel_y_raw.buf, LSM6DSO_REG_ACCEL_Y_AXIS_L, 2);
	if(status != HAL_OK)
		return status;

	status = lsm6dso_read_mult_reg(imu, accel_z_raw.buf, LSM6DSO_REG_ACCEL_Z_AXIS_L, 2);
	if(status != HAL_OK)
		return status;

	/* Setting imu struct values to converted measurements */
	imu->accel_data[LSM6DSO_X_AXIS] = accel_data_convert(accel_x_raw.data);
	imu->accel_data[LSM6DSO_Y_AXIS] = accel_data_convert(accel_y_raw.data);
	imu->accel_data[LSM6DSO_Z_AXIS] = accel_data_convert(accel_z_raw.data);

	return HAL_OK;
}

HAL_StatusTypeDef lsm6dso_read_gyro(lsm6dso_t *imu)
{
	union {
		uint8_t buf[2];
		int16_t data;
	} gyro_x_raw, gyro_y_raw, gyro_z_raw;
	HAL_StatusTypeDef status;

	/* Aquire raw data from registers */
	status = lsm6dso_read_mult_reg(imu, gyro_x_raw.buf, LSM6DSO_REG_GYRO_X_AXIS_L, 2);
	if(status != HAL_OK)
		return status;

	status = lsm6dso_read_mult_reg(imu, gyro_y_raw.buf, LSM6DSO_REG_GYRO_Y_AXIS_L, 2);
	if(status != HAL_OK)
		return status;

	status = lsm6dso_read_mult_reg(imu, gyro_z_raw.buf, LSM6DSO_REG_GYRO_Z_AXIS_L, 2);
	if(status != HAL_OK)
		return status;

	/* Setting imu struct values to converted measurements */
	imu->gyro_data[LSM6DSO_X_AXIS] = gyro_data_convert(gyro_x_raw);
	imu->gyro_data[LSM6DSO_Y_AXIS] = gyro_data_convert(gyro_y_raw);
	imu->gyro_data[LSM6DSO_Z_AXIS] = gyro_data_convert(gyro_z_raw);

	return HAL_OK;
}
