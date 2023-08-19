#include "m24c32.h"

uint16_t bytes_to_write(uint16_t size, uint16_t offset)
{
    if((size + offset) < M24C32_PAGE_SIZE) return size;
    else return M24C32_PAGE_SIZE - offset;
}

void eeprom_write(uint8_t page, uint16_t offset, uint8_t *data, uint16_t size)
{
    uint8_t start_page = page;
    uint8_t end_page = page + ((size + offset) / M24C32_PAGE_SIZE);
    uint8_t num_pages = end_page - start_page + 1;

    uint8_t data_pos = 0;

    for(int i = 0; i < num_pages; i++)
    {
        uint16_t mem_addr = start_page << 4 | offset;
        uint16_t bytes_remaining = bytes_to_write(size, offset);

        HAL_I2C_Mem_Write(i2c_handle, M24C32_I2C_ADDR, mem_addr, 2, &data[data_pos], bytes_remaining, 1000);
        start_page += 1;
        offset = 0;
        size = size - bytes_remaining;
        data_pos = bytes_remaining;

        HAL_Delay(5);
    }
}

void eeprom_read(uint8_t page, uint16_t offset, uint8_t *data, uint16_t size)
{
    uint8_t start_page = page;
    uint8_t end_page = page + ((size + offset) / M24C32_PAGE_SIZE);
    uint8_t num_pages = end_page - start_page + 1;

    uint8_t data_pos = 0;

    for(int i = 0; i < num_pages; i++)
    {
        uint16_t mem_addr = start_page << 4 | offset;
        uint16_t bytes_remaining = bytes_to_write(size, offset);

        HAL_I2C_Mem_Read(i2c_handle, M24C32_I2C_ADDR, mem_addr, 2, &data[data_pos], bytes_remaining, 1000);
        start_page += 1;
        offset = 0;
        size = size - bytes_remaining;
        data_pos = bytes_remaining;

        HAL_Delay(5);
    }
}

void eeprom_delete(uint8_t page, uint16_t offset, uint16_t size)
{
    uint8_t start_page = page;
    uint8_t end_page = page + ((size + offset) / M24C32_PAGE_SIZE);
    uint8_t num_pages = end_page - start_page + 1;
    uint8_t data = 0;
    uint8_t * ptr = &data;


    for(int i = 0; i < num_pages; i++)
    {
        uint16_t mem_addr = start_page << 4 | offset;
        uint16_t bytes_remaining = bytes_to_write(size, offset);

        HAL_I2C_Mem_Write(i2c_handle, M24C32_I2C_ADDR, mem_addr, 2, &ptr, bytes_remaining, 1000);
        start_page += 1;
        offset = 0;
        size = size - bytes_remaining;
        HAL_Delay(5);
    }
}

void eeprom_page_erase(uint8_t page)
{
    uint16_t mem_addr = (page << 4) | offset;
    uint8_t data = 0;
    uint8_t * ptr = &data;

    HAL_I2C_Mem_Write(i2c_handle, M24C32_I2C_ADDR, mem_addr, 2, &ptr, M24C32_PAGE_SIZE, 1000);
}