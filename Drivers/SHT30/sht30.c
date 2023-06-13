#include "sht30.h"
#include <stdio.h>
#include <stdbool.h>

/* Creates a new sht30 Driver*/
sht30_t* create_SHT30()
{
    sht30_t* sht30 = {0};
    reset(sht30);
    printf("SHT30 is reset.\n");
    printf("Heating element is enabled: %d\n", isHeaterEnabled(sht30));
    return sht30;
}

/* Destroys an sht30 Driver */
void destroy_SHT30(sht30_t* sht30)
{
    free(sht30);
}

/* Allows for private I2C writing with SHT30 */
void sht30Write(cmd_t *cmd, uint8_t num_bytes) 
{
    //TODO STM32 I2C write
}

/* Allows for private I2C reading with SHT30 */
bool SHT30read(uint8_t *msg, uint8_t num_bytes)
{
    //TODO STM32 I2C read
}

/* Reads the status Reg */
uint16_t readStatusReg(sht30_t* sht30)
{
    cmd_t cmd;
    cmd.cmdVal = SWITCHBYTES(SHT30_READSTATUS);

    SHT30write(&cmd, 2);
    uint8_t data[3];
    SHT30read(data, 3);
    uint16_t status = data[0];
    status <<= 8;
    status |= data[1];
    return status;
}

/* Resets the SHT30 */
void reset(sht30_t* sht30)
{
    cmd_t cmd;
    cmd.cmdVal = SWITCHBYTES(SHT30_SOFTRESET);
    SHT30write(&cmd, 2);
}

/* Checks if the heater is enabled */
bool isHeaterEnabled(sht30_t* sht30)
{
    uint16_t reg_val = readStatusReg(sht30);
    return (bool)((reg_val >> SHT30_REG_HEATER_BIT) & 0x01);
}

/* Enables or disables the heater */
void enableHeater(sht30_t* sht30, bool h)
{
    cmd_t cmd;
    cmd.cmdVal = h ? SWITCHBYTES(SHT30_HEATEREN) : SWITCHBYTES(SHT30_HEATERDIS);
    SHT30write(&cmd, 2);
}

/* Reads the temperature and humidity */
void getTempHumid(sht30_t* sht30, uint8_t *msg)
{
    cmd_t cmd;
    cmd.cmdVal = SWITCHBYTES(SHT30_START_CMD_WCS);
    SHT30write(&cmd, 2);
    SHT30read(msg, 6);
}