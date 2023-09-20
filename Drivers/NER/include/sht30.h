#ifndef sht30_h
#define sht30_h

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * https://www.mouser.com/datasheet/2/682/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital-971521.pdf  --Datasheet
 *
 */
#define SHT30_I2C_ADDR             0x44     /* If ADDR (pin2) is connected to VDD, 0x45 */
#define SHT30_READSTATUS           0xF32D   /* Read Out of Status Register */
#define SHT30_CLEARSTATUS          0x3041   /* Clear Status */
#define SHT30_SOFTRESET            0x30A2   /* Soft Reset */
#define SHT30_HEATEREN             0x306D   /* Heater Enable */
#define SHT30_HEATERDIS            0x3066   /* Heater Disable */
#define SHT30_REG_HEATER_BIT       0x0d     /* Status Register Heater Bit */

/*
 * Start measurement command with clock streching enabled and high repeatability.
 *  This is responsible for retrieving the temp and humidity in single shot mode
 */
#define SHT30_START_CMD_WCS        0x2C06

/* Start measurement command with clock streching disabled and high repeatability */
#define SHT30_START_CMD_NCS        0x2400


typedef struct
{
    I2C_HandleTypeDef *i2c_handle;
    uint16_t status_reg;
    uint16_t temp;
    uint16_t humidity;
    bool is_heater_enabled;
} sht30_t;

/**
 * @brief Initializes an SHT30 Driver
 *
 * @param sht30
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef sht30_init(sht30_t *sht30, I2C_HandleTypeDef *hi2c);

/**
 * @brief Resets the SHT30 chip
 *
 * @param sht30
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef sht30_reset(sht30_t *sht30);

/**
 * @brief Checks if the internal heater is enabled
 *
 * @param sht30
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef sht30_is_heater_enabled(sht30_t *sht30);

/**
 * @brief Toggles the status of the internal heater
 *
 * @param sht30
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef sht30_toggle_heater(sht30_t *sht30);

/**
 * @brief Retrieves the temperature and humidity
 *
 * @param sht30
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef sht30_get_temp_humid(sht30_t *sht30);

#endif