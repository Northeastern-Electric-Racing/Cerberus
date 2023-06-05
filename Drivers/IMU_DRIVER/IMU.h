/*
    LSM6DSOXTR IMU DRIVER Header File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/datasheet/lsm6dsox.pdf

    Author: Hamza Iqbal
*/

#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H
#include "stm32f4xx_hal.h"

#define IMU_I2C_ADDR        0x35
// Not sure if these are all needed, also not sure if more need ot be added
// For register descriptions reference datasheet pages 47 - 98
#define FUNC_CFG_ACCESS     0x01    // Enagle embedded functions register
#define INT1_CTRL           0x0D    // INT1 pin control, used for interrupts
#define INT2_CTRL           0x0E    // INT2 pin control, used for interrupts
#define CTRL1_XL            0x10    // Accelerometer Control Register
#define CTRL2_G             0x11    // Gyroscope Control Register
#define CTRL3_C             0x12    // Control Register
#define CTRL4_C             0x13    // Control Register
#define CTRL5_C             0x14    // Control Register
#define CTRL6_C             0x15    // Control Register
#define CTRL7_G             0x16    // Control Register
#define CTRL8_XL            0x17    // Accelerometer Control Register
#define CTRL9_XL            0x18    // Accelerometer Control Register
#define CTRL10_C            0x19    // Control Register
#define ALL_INT_SRC         0x1A    // Sourse Register for all interupsts
#define WAKE_UP_SRC         0x1B    // Wake up interupt source register
#define TAP_SRC             0x1C    // Tap Interrupt source register
#define D6D_SRC             0x1D    // 6-direction Interrupt source register
#define STATUS_REG          0x1E    // Status register
#define OUT_TEMP_L          0x20    // Temperature Data Low
#define OUT_TEMP_H          0x21    // Temperature Data High
#define OUTX_L_G            0x22    // Gyro pitch axis lower bits
#define OUTX_H_G            0x23    // Gyro pitch axis upper bits
#define OUTY_L_G            0x24    // Gyro roll axis lower bits
#define OUTY_H_G            0x25    // Gyro roll axis upper bits
#define OUTZ_L_G            0x26    // Gyro yaw axis lower bits
#define OUTZ_H_G            0x27    // Gyro yaw axis higher bits
#define OUTX_L_A            0x28    // Accelerometer X axis lower bits
#define OUTX_H_A            0x29    // Accelerometer X axis upper bits
#define OUTY_L_A            0x2A    // Accelerometer Y axis lower bits
#define OUTY_H_A            0x2B    // Accelerometer Y axis upper bits
#define OUTZ_L_A            0x2C    // Accelerometer Z axis lower bits
#define OUTZ_H_A            0x2D    // Accelerometer Z axis upper bits

/*
    SENSOR STRUCT
*/

typedef struct 
{
    // All values are 2s complement

    // Array to hold accelerometer data
    // Index 0: X axis
    // Index 1: Y axis
    // Index 2: Z axis
    int16_t accel_data[3];  

    // Array to hold gyroscopic data
    // Index 0: X axis
    // Index 1: Y axis
    // Index 2: Z axis
    int16_t gyro_data[3];

    // Integer for temp data
    int16_t temp_data;
} IMU_DATA;

// Initialization of the IMU / Setup
uint8_t IMU_init(IMU_DATA *imu);

// Data Aquisition
HAL_StatusTypeDef IMU_readAccel(IMU_DATA *imu);
HAL_StatusTypeDef IMU_readGyro(IMU_DATA *imu);
HAL_StatusTypeDef IMU_readTemp(IMU_DATA *imu);

// Low Level Functions for reading and writing to registers
// Read
HAL_StatusTypeDef IMU_readReg(IMU_DATA *imu, uint8_t *data, uint8_t reg);
HAL_StatusTypeDef IMU_readMultReg(IMU_DATA *imu, uint8_t *data, uint8_t reg, uint8_t length);
// Write
HAL_StatusTypeDef IMU_writeReg(IMU_DATA *imu, uint8_t reg, uint8_t *data);

#endif