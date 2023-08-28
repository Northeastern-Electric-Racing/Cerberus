#include "sht30.h"
#include <stdio.h>
#include <stdbool.h>

#define SWITCHBYTES(x)             (((x<<8) | (x>>8)) & 0xFFFF)      //Switches the first and second byte (This is required for all command values)

static HAL_StatusTypeDef sht30_read_reg(sht30_t *sht30, uint8_t *data, uint16_t reg, uint8_t size)
{
    return HAL_I2C_Mem_Read(sht30->i2c_handle, SHT30_I2C_ADDR, reg, I2C_MEMADD_SIZE_16BIT, data, size, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef sht30_write_reg(sht30_t *sht30, uint16_t reg, uint8_t *data, uint8_t size)
{
    return HAL_I2C_Mem_Write(sht30->i2c_handle, SHT30_I2C_ADDR, reg, I2C_MEMADD_SIZE_16BIT, data, size, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef sht30_handle_status_reg(sht30_t *sht30)
{
    HAL_StatusTypeDef status;
    uint8_t data[3];
    uint16_t register_status;
    
    status = sht30_write_reg(sht30, SWITCHBYTES(SHT30_READSTATUS), 0, 2);
    if (status != HAL_OK)
        return status;

    status = sht30_read_reg(sht30, data, SWITCHBYTES(SHT30_READSTATUS), 3);
    if (status != HAL_OK)
        return status;

    register_status = data[0];
    register_status <<= 8;
    register_status |= data[1];
    sht30->status_reg = register_status;
    return HAL_OK;
}

/**
 * @brief Calculates the CRC by using the polynomial x^8 + x^5 + x^4 + 1
 * @param data: the data to use to calculate the CRC
*/
static uint8_t calculate_crc(uint16_t data) {
    uint8_t crc = 0xFF, i;

    crc ^= (data >> 8);
    crc ^= (data & 0xFF);
    for (i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ 0x31;
        } else {
            crc = (crc << 1);
        }
    }
    return crc;
}

HAL_StatusTypeDef sht30_reset(sht30_t *sht30)
{
    return sht30_write_reg(sht30, SWITCHBYTES(SHT30_SOFTRESET), 0, 2);
}

HAL_StatusTypeDef sht30_init(sht30_t *sht30, I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef status;

    sht30->i2c_handle = hi2c;

    status = sht30_reset(sht30);
    if (status != HAL_OK)
        return status;

    //TODO: Replace with UART debug stream if necessary
    printf("SHT30 is reset.\n");
    printf("Heating element is enabled: %d\n", sht30_is_heater_enabled(sht30));
    
    return HAL_OK;
}

HAL_StatusTypeDef sht30_is_heater_enabled(sht30_t *sht30)
{
    HAL_StatusTypeDef status;
    status = sht30_handle_status_reg(sht30);
    if (status != HAL_OK)
        return status;

    sht30->is_heater_enabled = (bool)((sht30->status_reg >> SHT30_REG_HEATER_BIT) & 0x01);

    return HAL_OK;
}

HAL_StatusTypeDef sht30_toggle_heater(sht30_t *sht30)
{
    uint16_t cmdVal = sht30->is_heater_enabled ? SWITCHBYTES(SHT30_HEATEREN) : SWITCHBYTES(SHT30_HEATERDIS);
    HAL_StatusTypeDef status;

    status = sht30_write_reg(sht30, cmdVal, 0, 2);
    if (status != HAL_OK)
        return status;

    return sht30_is_heater_enabled(sht30);
}

HAL_StatusTypeDef sht30_get_temp_humid(sht30_t *sht30)
{
    HAL_StatusTypeDef status;

    union
    {
        struct __attribute__((packed))
        {
            uint16_t temp;
            uint8_t temp_crc;
            uint16_t humidity;
            uint8_t humidity_crc;
        } raw_data;
        uint8_t databuf[6];
    } data;

    uint16_t temp, humidity;

    status = sht30_write_reg(sht30, SWITCHBYTES(SHT30_START_CMD_WCS), 0, 2);
    if (status != HAL_OK)
        return status;

    status = sht30_read_reg(sht30, data.databuf, SWITCHBYTES(SHT30_START_CMD_WCS), 6);
    if (status != HAL_OK)
        return status;

    temp = data.raw_data.temp;

    if (data.raw_data.temp_crc != calculate_crc(temp))
        return HAL_ERROR;

    sht30->temp = temp;

    humidity = data.raw_data.humidity;
    if (data.raw_data.humidity_crc != calculate_crc(humidity))
        return HAL_ERROR;

    sht30->humidity = humidity;

    return HAL_OK;
}