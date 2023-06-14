/*
    LSM6DSOXTR IMU DRIVER Source File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/datasheet/lsm6dsox.pdf

    Author: Hamza Iqbal
*/
#include "imu.h"

/* IMU Intitialization */
uint8_t imu_init(IMU_DATA *imu, I2C_HandleTypeDef *i2cHandle)
{
    imu->i2cHandle = i2cHandle;

    imu->accel_data->x_axis = 0;
    imu->accel_data->y_axis = 0;
    imu->accel_data->z_axis = 0;

    imu->gyro_data->x_axis = 0;
    imu->gyro_data->y_axis = 0;
    imu->gyro_data->z_axis = 0;

    
    uint8_t error_num = 0;
    HAL_StatusTypeDef status;

    /* Quick check to make sure I2C is working */
    uint8_t* regData;
    status = imu_read_reg(imu, regData, LSM6D_REG_WHO_AM_I);
    err_num += (status != HAL_OK);
    if(*regData != 0x6C)
    {
        return 255;
    }
    
    /* Configure IMU Refer to datasheet page 56 - 57 */    

    /* Accelerometer configuration settings */
    int8_t odr_sel_accel = 0x08;
    int8_t fs_sel_accel = 0x02;
    int8_t lp_f2_enable_accel = 0x01;
    int8_t accel_config = (((odr_sel_accel << 4) | (fs_sel_accel << 2) | (lp_f2_enable_accel << 1)) << 1);
    int8_t *accel_config_ptr = &accel_config;
    status = imu_write_reg(imu, LSM6D_REG_ACCEL_CTRL, accel_config_ptr);
    err_num += (status != HAL_OK);

    /* Gyroscope configuration settings */
    int8_t odr_sel_gryo = 0x08;
    int8_t fs_sel_gyro = 0x02;
    int8_t fs_125_gryo = 0x00;
    int8_t *gyro_config = &(((odr_sel_gyro << 4) | (fs_sel_gyro << 2) | (fs_125_gyro << 1)) << 1);
    status = imu_write_reg(imu, LSM6D_REG_GYRO_CTRL, gyro_config);
    err_num += (status != HAL_OK);

    return err_num;

}

/* Data Aquisition Functions */

/* Read accelerometer data */
HAL_StatusTypeDef imu_read_accel(IMU_DATA *imu)
{
    *int16_t accel_x_raw, accel_y_raw, accel_z_raw;
    HAL_StatusTypeDef status;

    /* Getting raw data from registers */
    status = imu_read_mult_reg(imu, accel_x_raw, LSM6D_REG_ACCEL_X_AXIS_L, 2);
    if(status != HAL_OK) return status;

    status = imu_read_mult_reg(imu, accel_y_raw, LSM6D_REG_ACCEL_Y_AXIS_L, 2);
    if(status != HAL_OK) return status;

    status = imu_read_mult_reg(imu, accel_z_raw, LSM6D_REG_ACCEL_Z_AXIS_L, 2);
    if(status != HAL_OK) return status;

    /* Setting imu struct values to converted measurements */
    imu->accel_data->x_axis = accel_data_convert(*accel_x_raw);
    imu->accel_data->y_axis = accel_data_convert(*accel_y_raw);
    imu->accel_data->z_axis = accel_data_convert(*accel_z_raw);

    return status;
}

/* Read gyro data */
HAL_StatusTypeDef imu_read_gyro(IMU_DATA *imu)
{
    *int16_t gyro_x_raw, gyro_y_raw, gyro_z_raw;
    HAL_StatusTypeDef status;

    /* Aquire raw data from registers */
    status = imu_read_mult_reg(imu, gyro_x_raw, LSM6D_REG_GYRO_X_AXIS_L, 2);
    if(status != HAL_OK) return status;

    status = imu_read_mult_reg(imu, gyro_y_raw, LSM6D_REG_GYRO_Y_AXIS_L, 2);
    if(status != HAL_OK) return status;

    status = imu_read_mult_reg(imu, gyro_z_raw, LSM6D_REG_GYRO_Z_AXIS_L, 2);
    if(status != HAL_OK) return status;

    /* Setting imu struct values to converted measurements */
    imu->gyro_data->x_axis = accel_data_convert(*gyro_x_raw);
    imu->gyro_data->y_axis = accel_data_convert(*gyro_y_raw);
    imu->gyro_data->z_axis = accel_data_convert(*gyro_z_raw);

    return status;
}

/* Converts raw accel data to meters per second * 100 */
static int16_t accel_data_convert(raw_accel)
{
    int8_t upper = (raw_accel & 0x00FF) << 8;
    int8_t lower = (raw_accel & 0xFF00) >> 8;
    int16_t accel_int = upper | lower;
    int16_t accel_mps = int16_t((accel_int * ACCEL_RANGE * 1000) / REG_RESOLUTION);
    return accel_mps;
}

/* Converts raw gyro data to degrees per second * 100 */
static int16_t gyro_data_convert(gyro_accel)
{
    int8_t upper = (gyro_accel & 0x00FF) << 8;
    int8_t lower = (gyro_accel & 0xFF00) >> 8;
    int16_t gyro_int = upper | lower;
    int16_t gyro_dps = int16_t((gyro_int * GYRO_RANGE * 100) / REG_RESOLUTION);
    return gyro_dps;
}

/* 
Low Level Functions for reading and writing to registers 
(basically just abstracts away the HAL)
*/

/* Read from a single register */
static HAL_StatusTypeDef imu_read_reg(IMU_DATA *imu, uint8_t *data, uint8_t reg)
{
    return HAL_I2C_Mem_Read(imu->i2cHandle, IMU_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY)
}

/* Read from multiple registers */
static HAL_StatusTypeDef imu_read_mult_reg(IMU_DATA *imu, uint8_t *data, uint8_t reg, uint8_t length)
{
    return HAL_I2C_Mem_Read(imu->i2cHandle, IMU_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, length, HAL_MAX_DELAY)
}

/* Write to a single register */
static HAL_StatusTypeDef imu_write_reg(IMU_DATA *imu, uint8_t reg, uint8_t *data)
{
    return HAL_I2C_Mem_Write(imu->i2cHandle, IMU_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, )
}

