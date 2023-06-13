/*
    LSM6DSOXTR IMU DRIVER Source File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/datasheet/lsm6dsox.pdf

    Author: Hamza Iqbal
*/
#include "imu.h"

// IMU Intitialization
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

}

// Data Aquisition Functions

// Read Raw accelerometer data
HAL_StatusTypeDef imu_read_accel(IMU_DATA *imu)
{

}

// Read raw gyro data
HAL_StatusTypeDef imu_read_gyro(IMU_DATA *imu)
{

}

// Read raw temp data
HAL_StatusTypeDef imu_read_temp(IMU_DATA *imu)
{

}


// Low Level Functions for reading and writing to registers

// Read from a single register
static HAL_StatusTypeDef imu_read_reg(IMU_DATA *imu, uint8_t *data, uint8_t reg)
{
    return HAL_I2C_Mem_Read(imu->i2cHandle, IMU_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY)
}

// Read from multiple registers
static HAL_StatusTypeDef imu_read_mult_reg(IMU_DATA *imu, uint8_t *data, uint8_t reg, uint8_t length)
{
    return HAL_I2C_Mem_Read(imu->i2cHandle, IMU_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, length, HAL_MAX_DELAY)
}

// Write to a single register
static HAL_StatusTypeDef imu_write_reg(IMU_DATA *imu, uint8_t reg, uint8_t *data)
{
    return HAL_I2C_Mem_Write(imu->i2cHandle, IMU_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, )
}

