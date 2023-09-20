/*
	LSM6DSOXTR IMU DRIVER Header File
	Link to part Datasheet for reference:
	https://www.st.com/resource/en/datasheet/lsm6dsox.pdf

	Author: Hamza Iqbal
*/

#ifndef LSM6DSO_H
#define LSM6DSO_H
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

#define LSM6DSO_I2C_ADDRESS                 0x35 << 1   /* Shifted because datasheet said to */
// Not sure if these are all needed, also not sure if more need to be added
/* For register descriptions reference datasheet pages 47 - 98 */
#define LSM6DSO_REG_FUNC_CFG_ACCESS         0x01    /* Enable embedded functions register */
#define LSM6DSO_REG_INTERRUPT_CTRL_1        0x0D    /* INT1 pin control, used for interrupts */
#define LSM6DSO_REG_INTERRUPT_CTRL_2        0x0E    /* INT2 pin control, used for interrupts */
#define LSM6DSO_REG_DEVICE_ID               0x0F    /* Register for checking communication */
#define LSM6DSO_REG_ACCEL_CTRL              0x10    /* Accelerometer Control Register */
#define LSM6DSO_REG_GYRO_CTRL               0x11    /* Gyroscope Control Register */
#define LSM6DSO_REG_ALL_INTERRUPT_SRC       0x1A    /* Source Register for all interupsts */
#define LSM6DSO_REG_WAKEUP_INTERRUPT_SRC    0x1B    /* Wake up interupt source register */
#define LSM6DSO_REG_TAP_INTERRUPT_SRC       0x1C    /* Tap Interrupt source register */
#define LSM6DSO_REG_6D_INTERRUPT_SRC        0x1D    /* 6-direction Interrupt source register */
#define LSM6DSO_REG_STATUS                  0x1E    /* Status register */
#define LSM6DSO_REG_GYRO_X_AXIS_L           0x22    /* Gyro pitch axis lower bits */
#define LSM6DSO_REG_GYRO_X_AXIS_H           0x23    /* Gyro pitch axis upper bits */
#define LSM6DSO_REG_GYRO_Y_AXIS_L           0x24    /* Gyro roll axis lower bits */
#define LSM6DSO_REG_GYRO_Y_AXIS_H           0x25    /* Gyro roll axis upper bits */
#define LSM6DSO_REG_GYRO_Z_AXIS_L           0x26    /* Gyro yaw axis lower bits */
#define LSM6DSO_REG_GYRO_Z_AXIS_H           0x27    /* Gyro yaw axis higher bits */
#define LSM6DSO_REG_ACCEL_X_AXIS_L          0x28    /* Accelerometer X axis lower bits */
#define LSM6DSO_REG_ACCEL_X_AXIS_H          0x29    /* Accelerometer X axis upper bits */
#define LSM6DSO_REG_ACCEL_Y_AXIS_L          0x2A    /* Accelerometer Y axis lower bits */
#define LSM6DSO_REG_ACCEL_Y_AXIS_H          0x2B    /* Accelerometer Y axis upper bits */
#define LSM6DSO_REG_ACCEL_Z_AXIS_L          0x2C    /* Accelerometer Z axis lower bits */
#define LSM6DSO_REG_ACCEL_Z_AXIS_H          0x2D    /* Accelerometer Z axis upper bits */

/**
 * @brief Enumeration of axes of data available from the LSM6DSO IMU
 *
 */
enum lsm6dso_axes
{
	LSM6DSO_X_AXIS = 0,
	LSM6DSO_Y_AXIS = 1,
	LSM6DSO_Z_AXIS = 2
};

/**
 * @brief Struct containing data relevant to the LSM6DSO IMU
 *
 */
typedef struct
{
	I2C_HandleTypeDef *i2c_handle;

	uint8_t accel_config; //TODO: We should make these cfg packed unions with bitfield structs

	uint8_t gyro_config;

	int16_t accel_data[3];

	int16_t gyro_data[3];

} lsm6dso_t;

/**
 * @brief Initializes the LSM6DSO IMU
 *
 * @param imu
 * @param i2c_handle
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef lsm6dso_init(lsm6dso_t *imu, I2C_HandleTypeDef *i2c_handle);

/* IMU Setting Configuration */
/**
 * @brief Configures the accelerometer of the LSM6DSO IMU
 *
 * @param imu
 * @param odr_sel
 * @param fs_sel
 * @param lp_f2_enable
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef lsm6dso_accelerometer_config(lsm6dso_t *imu, int8_t odr_sel, int8_t fs_sel, int8_t lp_f2_enable);

/**
 * @brief Configures the accelerometer of the LSM6DSO IMU
 *
 * @param imu
 * @param odr_sel
 * @param fs_sel
 * @param fs_125
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef lsm6dso_gyroscope_config(lsm6dso_t *imu, int8_t odr_sel, int8_t fs_sel, int8_t fs_125);

/* Data Aquisition */
/**
 * @brief Retrieves accelerometer data from the LSM6DSO IMU
 *
 * @param imu
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef lsm6dso_read_accel(lsm6dso_t *imu);

/**
 * @brief Retreives the gyroscope data from the LSM6DSO IMU
 *
 * @param imu
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef lsm6dso_read_gyro(lsm6dso_t *imu);

#endif // LSM6DSO_H
