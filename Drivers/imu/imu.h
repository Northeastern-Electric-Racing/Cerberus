/*
    LSM6DSOXTR IMU DRIVER Header File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/datasheet/lsm6dsox.pdf

    Author: Hamza Iqbal
*/

#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H
#include "stm32f4xx_hal.h"

#define IMU_I2C_ADDRESS                 0x35 << 1   /* Shifted because datasheet said to */
// Not sure if these are all needed, also not sure if more need ot be added
/* For register descriptions reference datasheet pages 47 - 98 */
#define LSM6D_REG_FUNC_CFG_ACCESS       0x01    /* Enable embedded functions register */
#define LSM6D_REG_INTERRUPT_CTRL_1      0x0D    /* INT1 pin control, used for interrupts */
#define LSM6D_REG_INTERRUPT_CTRL_2      0x0E    /* INT2 pin control, used for interrupts */
#define LSM6D_REG_DEVICE_ID             0x0F    /* Register for checking communication */
#define LSM6D_REG_ACCEL_CTRL            0x10    /* Accelerometer Control Register */
#define LSM6D_REG_GYRO_CTRL             0x11    /* Gyroscope Control Register */
#define LSM6D_REG_ALL_INTERRUPT_SRC     0x1A    /* Source Register for all interupsts */
#define LSM6D_REG_WAKEUP_INTERRUPT_SRC  0x1B    /* Wake up interupt source register */
#define LSM6D_REG_TAP_INTERRUPT_SRC     0x1C    /* Tap Interrupt source register */
#define LSM6D_REG_6D_INTERRUPT_SRC      0x1D    /* 6-direction Interrupt source register */
#define LSM6D_REG_STATUS                0x1E    /* Status register */
#define LSM6D_REG_GYRO_PITCH_L          0x22    /* Gyro pitch axis lower bits */
#define LSM6D_REG_GYRO_PITCH_H          0x23    /* Gyro pitch axis upper bits */
#define LSM6D_REG_GYRO_ROLL_L           0x24    /* Gyro roll axis lower bits */
#define LSM6D_REG_GYRO_ROLL_H           0x25    /* Gyro roll axis upper bits */
#define LSM6D_REG_GYRO_YAW_L            0x26    /* Gyro yaw axis lower bits */
#define LSM6D_REG_GYRO_YAW_H            0x27    /* Gyro yaw axis higher bits */
#define LSM6D_REG_ACCEL_X_AXIS_L        0x28    /* Accelerometer X axis lower bits */
#define LSM6D_REG_ACCEL_X_AXIS_H        0x29    /* Accelerometer X axis upper bits */
#define LSM6D_REG_ACCEL_Y_AXIS_L        0x2A    /* Accelerometer Y axis lower bits */
#define LSM6D_REG_ACCEL_Y_AXIS_H        0x2B    /* Accelerometer Y axis upper bits */
#define LSM6D_REG_ACCEL_Z_AXIS_L        0x2C    /* Accelerometer Z axis lower bits */
#define LSM6D_REG_ACCEL_Z_AXIS_H        0x2D    /* Accelerometer Z axis upper bits */


/* Resolution of the sensor registers */
#define REG_RESOLUTION                  32768   /* Half the range of a 16 bit signed integer */
#define ACCEL_RANGE                     4       /* The range of values in g's returned from accelerometer */
#define GYRO_RANGE                      1000    /* The range of values from the gyro in dps */


/* Converts array indices to axes for ease of reading */
enum
{
    IMU_X_AXIS = 0;
    IMU_Y_AXIS = 1;
    IMU_Z_AXIS = 2;
}

/* Fault codes */
enum
{
    FAULTED = 255;
    CLEAR   = 0;
}

/* SENSOR STRUCT */
typedef struct 
{
    
    I2C_HandleTypeDef *i2c_handle;

    int8_t *accel_config;

    int8_t *gyro_config;

    int16_t accel_data[3];

    int16_t gyro_data[3];

} LSM6D_t;

/* Initialization of the IMU / Setup */
uint8_t LSM6D_init(LSM6D_t *imu);

/* IMU Setting Configuration */
void LSM6D_accelerometer_config(LSM6D_t *imu, int8_t odr_sel, int8_t fs_sel, int8_t lp_f2_enable);
void LSM6D_gyroscope_config(LSM6D_t *imu, int8_t odr_sel, int8_t fs_sel, int8_t fs_125);

/* Data Aquisition */
HAL_StatusTypeDef LSM6D_read_accel(LSM6D_t *imu);
HAL_StatusTypeDef LSM6D_read_gyro(LSM6D_t *imu);

#endif
