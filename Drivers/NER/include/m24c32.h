#ifndef M24C32_H
#define M24C32_H

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

#define M24C32_I2C_ADDR     0x50
#define M24C32_NUM_PAGE     250
#define M24C32_PAGE_SIZE    32

I2C_HandleTypeDef *i2c_handle;

void EEPROM_Write(uint8_t page, uint16_t offset, uint8_t *data, uint16_t size);

void EEPROM_Read(uint8_t page, uint16_t offset, uint8_t *data, uint16_t size);

void EEPROM_Delete(uint8_t page, uint16_t offset, uint16_t size);

void EEPROM_Page_Erase(uint8_t page);



#endif