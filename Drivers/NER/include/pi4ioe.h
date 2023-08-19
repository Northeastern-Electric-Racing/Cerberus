/*
	PI4IOE5V9535ZDEX GPIO Expander Header File
	Link to part Datasheet for reference:
	https://www.diodes.com/assets/Datasheets/PI4IOE5V9535.pdf

	Author: Dylan Donahue
*/

#ifndef PI4IOE_H
#define PI4IOE_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

// all of these addresses are complete bullshit and wrong plz fix when know :) and remove this comment
// the values should be correct though, just the order is wrong

#define PI4IOE_I2C_ADDR 	0x20 //? @hamza does HAL driver handle shifting for read/write bit automatically? if not should be added here

#define IO_CONFIG_REG 		0x06 
#define IO_CONFIG_BUF 		0x10 /* = 0001 0000, bit 4 = TSMS = input */ 
#define INPUT0_REG 			0x00
#define INPUT1_REG 			0x01
#define OUTPUT0_REG 		0x02
#define OUTPUT1_REG 		0x03

#define PUMP_CTRL 			0x00
#define RADFAN_CTRL 		0x01
#define BRKLIGHT_CTRL 		0x02
#define BATBOXFAN_CTRL 		0x03
#define TSMS_CTRL 			0x04
#define SMBALERT	 		0x05


typedef struct
{
	I2C_HandleTypeDef *i2c_handle;
	uint8_t port_config;

} pi4ioe_t;

/**
 * @brief Initializes the PI4IOE5V9535ZDEX GPIO Expander with current device connections
 * @note needs update with any new device connections
 * 
 * @param gpio
 * @param hi2c
 * @return HAL_StatusTypeDef 
 */
 HAL_StatusTypeDef pio4oe_init(pi4ioe_t *gpio, I2C_HandleTypeDef *hi2c);

/**
 * @brief Writes to the PI4IOE5V9535ZDEX GPIO Expander
 * 
 * @param device 
 * @param val
 * @param i2c_handle
 * @return HAL_StatusTypeDef 
 */
 HAL_StatusTypeDef pio4oe_Write(uint8_t device, uint8_t val, I2C_HandleTypeDef *i2c_handle);

/**
 * @brief Reads from the PI4IOE5V9535ZDEX GPIO Expander
 * @note always reads from both input registers
 * 
 * @param buf
 * @param i2c_handle 
 */
 HAL_StatusTypeDef pio4oe_read(uint8_t *buf, I2C_HandleTypeDef *i2c_handle);


#endif // PI4IOE_H