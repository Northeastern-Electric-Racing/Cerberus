/*
    LSM6DSOXTR IMU DRIVER Source File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/datasheet/lsm6dsox.pdf

    Author: Hamza Iqbal
*/
#include "imu.h"

/* IMU Intitialization */
uint8_t LSM6D_init(IMU_DATA *imu, I2C_HandleTypeDef *i2cHandle)
{
    imu->i2cHandle = i2cHandle;

    imu->accel_data[IMU_X_AXIS] = 0;
    imu->accel_data[IMU_Y_AXIS] = 0;
    imu->accel_data[IMU_Z_AXIS] = 0;

    imu->gyro_data[IMU_X_AXIS] = 0;
    imu->gyro_data[IMU_Y_AXIS] = 0;
    imu->gyro_data[IMU_Z_AXIS] = 0;

    
    uint8_t error_num = 0;
    HAL_StatusTypeDef status;

    /* Quick check to make sure I2C is working */
    fault_check = LSM6D_ping_imu(imu)
    if(fault_check == FAULTED) return FAULTED
    
    /* 
        Configure IMU 
        Refer to datasheet pages 56-57
    */    

    LSM6D_accelerometer_config(imu, odr_sel=0x08, fs_sel=0x02, lp_f2_enable=0x01)
    status = imu_write_reg(imu, LSM6D_REG_ACCEL_CTRL, imu->accel_config);
    err_num += (status != HAL_OK);

    LSM6D_gyroscope_config(imu, odr_sel = 0x08, fs_sel = 0x02, fs_125 = 0x00)
    status = imu_write_reg(imu, LSM6D_REG_GYRO_CTRL, imu->gyro_config);
    err_num += (status != HAL_OK);

    return err_num;

}
/*
    Configure Accelerometer and Gyro
*/

/* Configures Accelerometer Settings */
void LSM6D_accelerometer_config(LSM6D_t *imu, int8_t odr_sel, int8_t fs_sel, int8_t lp_f2_enable)
{
    int8_t config = (((odr_sel << 4) | (fs_sel << 2) | (lp_f2_enable << 1)) << 1);
    imu->accel_config = &config;
}

/* Configures Gyro Settings */
void LSM6D_gyroscope_config(LSM6D_t *imu, int8_t odr_sel, int8_t fs_sel, int8_t fs_125)
{
    int8_t config = (((odr_sel << 4) | (fs_sel << 2) | (fs_125 << 1)) << 1);
    imu->gyro_config = &config;
}


/* 
    Data Aquisition Functions 
*/

/* Read accelerometer data */
HAL_StatusTypeDef LSM6D_read_accel(LSM6D_t *imu)
{
    *int16_t accel_x_raw, accel_y_raw, accel_z_raw;
    HAL_StatusTypeDef status;

    /* Getting raw data from registers */
    status = LSM6D_read_mult_reg(imu, accel_x_raw, LSM6D_REG_ACCEL_X_AXIS_L, 2);
    if(status != HAL_OK) return status;

    status = LSM6D_read_mult_reg(imu, accel_y_raw, LSM6D_REG_ACCEL_Y_AXIS_L, 2);
    if(status != HAL_OK) return status;

    status = LSM6D_read_mult_reg(imu, accel_z_raw, LSM6D_REG_ACCEL_Z_AXIS_L, 2);
    if(status != HAL_OK) return status;

    /* Setting imu struct values to converted measurements */
    imu->accel_data[IMU_X_AXIS] = accel_data_convert(*accel_x_raw);
    imu->accel_data[IMU_Y_AXIS] = accel_data_convert(*accel_y_raw);
    imu->accel_data[IMU_Z_AXIS] = accel_data_convert(*accel_z_raw);

    return status;
}

/* Read gyro data */
HAL_StatusTypeDef LSM6D_read_gyro(LSM6D_t *imu)
{
    *int16_t gyro_x_raw, gyro_y_raw, gyro_z_raw;
    HAL_StatusTypeDef status;

    /* Aquire raw data from registers */
    status = LSM6d_read_mult_reg(imu, gyro_x_raw, LSM6D_REG_GYRO_X_AXIS_L, 2);
    if(status != HAL_OK) return status;

    status =  LSM6D_read_mult_reg(imu, gyro_y_raw, LSM6D_REG_GYRO_Y_AXIS_L, 2);
    if(status != HAL_OK) return status;

    status = LSM6D_read_mult_reg(imu, gyro_z_raw, LSM6D_REG_GYRO_Z_AXIS_L, 2);
    if(status != HAL_OK) return status;

    /* Setting imu struct values to converted measurements */
    imu->gyro_data[IMU_X_AXIS] = accel_data_convert(*gyro_x_raw);
    imu->gyro_data[IMU_Y_AXIS] = accel_data_convert(*gyro_y_raw);
    imu->gyro_data[IMU_Z_AXIS] = accel_data_convert(*gyro_z_raw);

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

static int8_t LSM6D_ping_imu(LSM6D_t *imu)
{
    uint8_t *reg_data;
    status = LSM6D_read_reg(imu, reg_data, LSM6D_REG_DEVICE_ID);
    if((status != HAL_OK) || (*reg_data != 0x6C))
    {
        return FAULTED;
    }
}

/* 
Low Level Functions for reading and writing to registers 
(basically just abstracts away the HAL)
*/

/* Read from a single register */
static HAL_StatusTypeDef LSM6D_read_reg(LSM6D_t *imu, uint8_t *data, uint8_t reg)
{
    return HAL_I2C_Mem_Read(imu->i2cHandle, LSM6D_REG_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY)
}

/* Read from multiple registers */
static HAL_StatusTypeDef LSM6D_read_mult_reg(LSM6D_t *imu, uint8_t *data, uint8_t reg, uint8_t length)
{
    return HAL_I2C_Mem_Read(imu->i2cHandle, LSM6D_REG_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, length, HAL_MAX_DELAY)
}

/* Write to a single register */
static HAL_StatusTypeDef LSM6D_write_reg(LSM6D_t *imu, uint8_t reg, uint8_t *data)
{
    return HAL_I2C_Mem_Write(imu->i2cHandle, LSM6D_REG_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data)
}

