/*
    LSM6DSOXTR IMU DRIVER Source File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/datasheet/lsm6dsox.pdf

    Author: Hamza Iqbal
*/
#include "IMU.h"

// IMU Intitialization
uint8_t IMU_init(IMU_DATA *imu)
{

}

// Data Aquisition Functions

// Read Raw accelerometer data
HAL_StatusTypeDef IMU_readAccel(IMU_DATA *imu)
{

}

// Read raw gyro data
HAL_StatusTypeDef IMU_readGyro(IMU_DATA *imu)
{

}

// Read raw temp data
HAL_StatusTypeDef IMU_readTemp(IMU_DATA *imu)
{

}


// Low Level Functions for reading and writing to registers

// Read from a single register
HAL_StatusTypeDef IMU_readReg(IMU_DATA *imu, uint8_t *data, uint8_t reg)
{
    HAL_I2C_Mem_Read()
}

// Read from multiple registers
HAL_StatusTypeDef IMU_readMultReg(IMU_DATA *imu, uint8_t *data, uint8_t reg, uint8_t length)
{

}

// Write to a single register
HAL_StatusTypeDef IMU_writeReg(IMU_DATA *imu, uint8_t reg, uint8_t *data)
{

}
