#ifndef M24C32_H
#define M24C32_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

#define M24C32_I2C_ADDR     0x50
#define M24C32_NUM_PAGE     250
#define M24C32_PAGE_SIZE    32

I2C_HandleTypeDef *i2c_handle;

HAL_StatusTypeDef eeprom_write(uint8_t page, uint16_t offset, uint8_t *data, uint16_t size);

HAL_StatusTypeDef eeprom_read(uint8_t page, uint16_t offset, uint8_t *data, uint16_t size);

HAL_StatusTypeDef eeprom_delete(uint8_t page, uint16_t offset, uint16_t size);

HAL_StatusTypeDef eeprom_page_erase(uint8_t page);

#endif // M24C32_H