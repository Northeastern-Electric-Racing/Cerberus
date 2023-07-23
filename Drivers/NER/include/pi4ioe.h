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



#define BREAKLIGHT_ADDR 	0x40
#define RADFAN_ADDR			0x42
#define RADPUMP_ADDR 		0x44
#define TSMS_ADDR 			0x46
#define BATBOXFAN_ADDR 		0x48
#define SPARE1_ADDR 		0x4A
#define SPARE2_ADDR 		0x4C
#define SPARE3_ADDR 		0x4E


#endif // PI4IOE_H