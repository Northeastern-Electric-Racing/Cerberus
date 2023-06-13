/*
    LSM6DSOXTR IMU DRIVER Header File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/datasheet/lsm6dsox.pdf

    Author: Hamza Iqbal
*/

#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H
#include "stm32f4xx_hal.h"

#define IMU_I2C_ADDRESS                 0x35 << 1   // Shifted because datasheet said to
// Not sure if these are all needed, also not sure if more need ot be added
// For register descriptions reference datasheet pages 47 - 98
#define LSM6D_REG_FUNC_CFG_ACCESS       0x01    // Enable embedded functions register
#define LSM6D_REG_INTERRUPT_CTRL_1      0x0D    // INT1 pin control, used for interrupts
#define LSM6D_REG_INTERRUPT_CTRL_2      0x0E    // INT2 pin control, used for interrupts
#define LSM6D_REG_ACCEL_CTRL            0x10    // Accelerometer Control Register
#define LSM6D_REG_GYRO_CTRL             0x11    // Gyroscope Control Register
#define LSM6D_REG_ALL_INTERRUPT_SRC     0x1A    // Source Register for all interupsts
#define LSM6D_REG_WAKEUP_INTERRUPT_SRC  0x1B    // Wake up interupt source register
#define LSM6D_REG_TAP_INTERRUPT_SRC     0x1C    // Tap Interrupt source register
#define LSM6D_REG_6D_INTERRUPT_SRC      0x1D    // 6-direction Interrupt source register
#define LSM6D_REG_STATUS                0x1E    // Status register
#define LSM6D_REG_GYRO_PITCH_L          0x22    // Gyro pitch axis lower bits
#define LSM6D_REG_GYRO_PITCH_H          0x23    // Gyro pitch axis upper bits
#define LSM6D_REG_GYRO_ROLL_L           0x24    // Gyro roll axis lower bits
#define LSM6D_REG_GYRO_ROLL_H           0x25    // Gyro roll axis upper bits
#define LSM6D_REG_GYRO_YAW_L            0x26    // Gyro yaw axis lower bits
#define LSM6D_REG_GYRO_YAW_H            0x27    // Gyro yaw axis higher bits
#define LSM6D_REG_ACCEL_X_AXIS_L        0x28    // Accelerometer X axis lower bits
#define LSM6D_REG_ACCEL_X_AXIS_H        0x29    // Accelerometer X axis upper bits
#define LSM6D_REG_ACCEL_Y_AXIS_L        0x2A    // Accelerometer Y axis lower bits
#define LSM6D_REG_ACCEL_Y_AXIS_H        0x2B    // Accelerometer Y axis upper bits
#define LSM6D_REG_ACCEL_Z_AXIS_L        0x2C    // Accelerometer Z axis lower bits
#define LSM6D_REG_ACCEL_Z_AXIS_H        0x2D    // Accelerometer Z axis upper bits

// Struct to hold accelerometer data
typedef struct
{
    int16_t x_axis;
    int16_t y_axis;
    int16_t z_axis;
} accel_t;
    

// Struct to hold gyroscopic data
typedef struct
{
    int16_t x_axis;
    int16_t y_axis;
    int16_t z_axis;
} gyro_t;


/*
    SENSOR STRUCT
*/

typedef struct 
{
    
    I2C_HandleTypeDef *i2cHandle;

    accel_t *accel_data;

    gyro_t *gyro_data;

} imu_t;

// Initialization of the IMU / Setup
uint8_t imu_init(imu_t *imu);

// Data Aquisition
HAL_StatusTypeDef imu_read_accel(imu_t *imu);
HAL_StatusTypeDef imu_read_gyro(imu_t *imu);
HAL_StatusTypeDef imu_read_temp(imu_t *imu);

#endif
