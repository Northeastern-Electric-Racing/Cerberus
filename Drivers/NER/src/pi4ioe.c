/*
	PI4IOE5V9535ZDEX GPIO Expander Source File
	Link to part Datasheet for reference:
	https://www.diodes.com/assets/Datasheets/PI4IOE5V9535.pdf

	Author: Dylan Donahue
*/
#include "pi4ioe.h"

HAL_StatusTypeDef pi4ioe_init(pi4ioe_t *gpio, I2C_HandleTypeDef *i2c_handle)
{
	gpio->i2c_handle = i2c_handle;
	gpio->port_config = IO_CONFIG_BUF;

	uint8_t buf[2] = {IO_CONFIG_REG, IO_CONFIG_BUF};

	/* write to config reg setting TSMS to input, rest to output */
	return HAL_I2C_Master_Transmit(i2c_handle, PI4IOE_I2C_ADDR, buf, 2, HAL_MAX_DELAY);
}

 HAL_StatusTypeDef PI4IOE_Write(uint8_t device, uint8_t val, I2C_HandleTypeDef *i2c_handle)
 {
	uint8_t reg;

	if (device > 7) reg = OUTPUT1_REG;
	else reg = OUTPUT0_REG;
		
	uint8_t buf[2] = {reg, val << device};
	return HAL_I2C_Master_Transmit(i2c_handle, PI4IOE_I2C_ADDR, buf, 2, HAL_MAX_DELAY);

 }

  HAL_StatusTypeDef PI4IOE_Read(uint8_t *buf, I2C_HandleTypeDef *i2c_handle)
  {
	uint8_t reg = INPUT0_REG;
	
	HAL_I2C_Master_Transmit(i2c_handle, PI4IOE_I2C_ADDR, &reg, 1, HAL_MAX_DELAY);
	return HAL_I2C_Master_Receive(i2c_handle, PI4IOE_I2C_ADDR, buf, 2, HAL_MAX_DELAY);

  }
