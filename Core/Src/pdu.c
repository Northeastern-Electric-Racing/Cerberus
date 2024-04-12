#include "pdu.h"
#include "serial_monitor.h"
#include <assert.h>
#include <stdlib.h>

#define PUMP_CTRL	   0x00
#define RADFAN_CTRL	   0x01
#define BRKLIGHT_CTRL  0x02
#define BATBOXFAN_CTRL 0x03
#define TSMS_CTRL	   0x04
#define SMBALERT	   0x05
#define MUTEX_TIMEOUT  osWaitForever /* ms */

#define SHUTDOWN_ADDR  0x20
#define CTRL_ADDR      0x24

static osMutexAttr_t pdu_mutex_attributes;
#include <stdio.h>
pdu_t* init_pdu(I2C_HandleTypeDef* hi2c)
{
	assert(hi2c);

	/* Create PDU struct */
	pdu_t* pdu = malloc(sizeof(pdu_t));
	assert(pdu);

	pdu->hi2c = hi2c;

	/* Initialize Shutdown GPIO Expander */
	pdu->shutdown_expander = malloc(sizeof(max7314_t));
    assert(pdu->shutdown_expander);
    pdu->shutdown_expander->dev_addr = SHUTDOWN_ADDR;
	if (max7314_init(pdu->shutdown_expander, pdu->hi2c)) {
		serial_print("\n\rshutdown init fail\n\r");
	    free(pdu->shutdown_expander);
	    free(pdu);
	    return NULL;
	}

	serial_print("I AM RUNNING");

	// /* Initialize Control GPIO Expander */
	pdu->ctrl_expander = malloc(sizeof(max7314_t));
    assert(pdu->ctrl_expander);
    pdu->ctrl_expander->dev_addr = CTRL_ADDR;
	if (max7314_init(pdu->ctrl_expander, pdu->hi2c)) {
	 	serial_print("\n\rctrl exp fail\n\r");
		free(pdu->ctrl_expander);
		free(pdu->shutdown_expander);
		free(pdu);
		return NULL;
	}

    // set pins 0-3 to outputs

    // debugging: endianness of this is probably reversed
    // uint8_t pin_configs = 0b11110000;
    // if (max7314_set_pin_modes(pdu->ctrl_expander, MAX7314_PINS_0_TO_7, &pin_configs)) {
	// 	serial_print("set pin modes fail");
    //     free(pdu->ctrl_expander);
    //     free(pdu->shutdown_expander);
    //     free(pdu);
    //     return NULL;
    // }

	// serial_print("IRAN");
	// if (write_pump(pdu, false) != 0) {
	// 	serial_print("WRITE PUMP FAIL\n");
	// }
	// write_fan_radiator(pdu, false);
	// write_brakelight(pdu, false) ;
	// write_fan_battbox(pdu, false) ;

	/* Create Mutex */
	pdu->mutex = osMutexNew(&pdu_mutex_attributes);
	assert(pdu->mutex);

	return pdu;
}

int8_t write_pump(pdu_t* pdu, bool status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, osWaitForever);
	if (stat)
		return stat;

	/* write pump over i2c */
    HAL_StatusTypeDef error = max7314_set_pin_state(pdu->ctrl_expander, PUMP_CTRL, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_fan_radiator(pdu_t* pdu, bool status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* write radiator over i2c */
    HAL_StatusTypeDef error = max7314_set_pin_state(pdu->ctrl_expander, RADFAN_CTRL, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_brakelight(pdu_t* pdu, bool status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* write brakelight over i2c */
    HAL_StatusTypeDef error = max7314_set_pin_state(pdu->ctrl_expander, BRKLIGHT_CTRL, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_fan_battbox(pdu_t* pdu, bool status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* write fan over i2c */
    HAL_StatusTypeDef error = max7314_set_pin_state(pdu->ctrl_expander, BATBOXFAN_CTRL, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t read_fuse(pdu_t* pdu, fuse_t fuse, bool* status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	uint16_t pin;
	switch (fuse) 
	{
		case FUSE_BATTBOX:
			pin = 4;
			break;
		case FUSE_LVBOX:
			pin = 5;
			break;
		case FUSE_FAN_RADIATOR:
			pin = 6;
			break;
		case FUSE_MC:
			pin = 7;
			break;
		case FUSE_FAN_BATTBOX:
			pin = 8;
			break;
		case FUSE_PUMP:
			pin = 9;
			break;
		case FUSE_DASHBOARD:
			pin = 10;
			break;
		case FUSE_BRAKELIGHT:
			pin = 11;
			break;
		case FUSE_BRB:
			pin = 12;
			break;
		default:
			return -1;
	}

	HAL_StatusTypeDef error = max7314_read_pin_state(pdu->ctrl_expander, pin, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }
	
	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t read_tsms_sense(pdu_t* pdu, bool* status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* read pin over i2c */
    uint8_t tsms_pin = 14;
    HAL_StatusTypeDef error = max7314_read_pin_state(pdu->ctrl_expander, tsms_pin, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t read_shutdown(pdu_t* pdu, shutdown_stage_t stage, bool* status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	uint16_t pin;
	switch (stage) 
	{
		case CKPT_BRB_CLR:
			pin = 0;
			break;
		case BMS_OK:
			pin = 2;
			break;
		case INTERTIA_SW_OK:
			pin = 3;
			break;
		case SPARE_GPIO1_OK:
			pin = 4;
			break;
		case IMD_OK:
			pin = 5;
			break;
		case BPSD_OK:
			pin = 8;
			break;
		case BOTS_OK:
			pin = 13;
			break;
		case HVD_INTLK_OK:
			pin = 14;
			break;
		case HVC_INTLK_OK:
			pin = 15;
			break;
		default:
			return -1;
	}
	
	// read pin over i2c
    HAL_StatusTypeDef error = max7314_read_pin_state(pdu->shutdown_expander, pin, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }

	osMutexRelease(pdu->mutex);
	return 0;
}
