/**
 * @file sht30.h
 * @brief 
 */
#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>

//SHT30-DIS Humidity Sensor
/**
 * https://www.mouser.com/datasheet/2/682/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital-971521.pdf  --Datasheet
 * 
 */

#define SWITCHBYTES(x)             (((x<<8) | (x>>8)) & 0xFFFF)      //Switches the first and second byte (This is required for all command values)

#define SHT30_I2C_ADDR             0x44     //If ADDR (pin2) is connected to VDD, 0x45

#define SHT30_START_CMD_WCS        0x2C06   //Start measurement command with clock streching enabled and high repeatability
#define SHT30_START_CMD_NCS        0x2400   //Start measurement command with clock streching disabled and high repeatability 
#define SHT30_READSTATUS           0xF32D   //Read Out of Status Register
#define SHT30_CLEARSTATUS          0x3041   //Clear Status
#define SHT30_SOFTRESET            0x30A2   //Soft Reset
#define SHT30_HEATEREN             0x306D   //Heater Enable
#define SHT30_HEATERDIS            0x3066   //Heater Disable

#define SHT30_REG_HEATER_BIT       0x0d     //Status Register Heater Bit

typedef struct
{
    uint16_t cmdVal;
    uint8_t cmd2Bytes[2];
} cmd_t;

typedef struct
{
    uint16_t statusReg;
} sht30_t;

sht30_t* create_SHT30();

void SHT30write(cmd_t *cmd, uint8_t num_bytes);

bool SHT30read(uint8_t *msg, uint8_t num_bytes);

void destroy_SHT30(sht30_t* sht30);

uint16_t readStatusReg(sht30_t* sht30);

void reset(sht30_t* sht30);

bool isHeaterEnabled(sht30_t* sht30);

void enableHeater(sht30_t* sht30, bool h);

void getTempHumid(sht30_t* sht30, uint8_t *msg);

#endif