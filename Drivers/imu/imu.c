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

    /* Quick check to make sure I2C is working work */
    uint8_t* regData;
    status = imu_read_reg(imu, regData, LSM6D_REG_WHO_AM_I);
    err_num += (status != HAL_OK);
    if(*regData != 0x6C)
    {
        return 255;
    }
    
    /* Configure IMU Refer to datasheet page 56 - 57 */

    /* Accelerometer configuration settings */
    int8_t odr_sel_accel = 0x8;
    int8_t fs_sel_accel = 0x2;
    int8_t lp_f2_enable_accel = 0x01;
    int8_t *accel_config = &(((odr_sel_accel << 4) | (fs_sel_accel << 2) | (lp_f2_enable_accel << 1)) << 1);
    status = imu_write_reg(imu, LSM6D_REG_ACCEL_CTRL, accel_config);
    err_num += (status != HAL_OK);
    /* Gyroscope configuration settings */
    int8_t odr_sel_gryo = 0x8;
    int8_t fs_sel_gyro = 0x2;
    int8_t fs_125_gryo = 0x0;
    int8_t *gyro_config = &(((odr_sel_gyro << 4) | (fs_sel_gyro << 2) | (fs_125_gyro << 1)) << 1);
    status = imu_write_reg(imu, LSM6D_REG_GYRO_CTRL, gyro_config);
    err_num += (status != HAL_OK);

    return err_num;

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

