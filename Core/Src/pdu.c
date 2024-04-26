#include "pdu.h"
#include "serial_monitor.h"
#include <assert.h>
#include <stdlib.h>

#define PUMP_CTRL	   0
#define RADFAN_CTRL	   1
#define BRKLIGHT_CTRL  2
#define BATBOXFAN_CTRL 3
#define RTDS_CTRL	   15
#define TSMS_CTRL	   0x04
#define SMBALERT	   0x05
#define RTDS_CTRL	   15
#define MUTEX_TIMEOUT  osWaitForever /* ms */

#define SHUTDOWN_ADDR  0x20
#define CTRL_ADDR      0x24
#define RTDS_DURATION	1000

#define PIN_MODE_OUTPUT 0
#define PIN_MODE_INPUT  1

static osMutexAttr_t pdu_mutex_attributes;

static void rtds_shutoff_cb(void* pv_params)
{
	pdu_t *pdu = (pdu_t *)pv_params;
	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return;

	/* write RTDS over i2c */
    HAL_StatusTypeDef error = max7314_set_pin_state(pdu->ctrl_expander, RTDS_CTRL, false);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return;
    }

	osMutexRelease(pdu->mutex);
}

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
	max7314_init(pdu->shutdown_expander, pdu->hi2c);

	// Blink disabled, global intensity enabled
	uint8_t config_data = 0b01001100;
    HAL_StatusTypeDef status = max7314_write_config(pdu->shutdown_expander, &config_data);
    if (status != HAL_OK) {
        serial_print("\n\rshutdown init fail\n\r");
	    free(pdu->shutdown_expander);
	    free(pdu);
	    return NULL;
    }

	/* Initialize Control GPIO Expander */
	pdu->ctrl_expander = malloc(sizeof(max7314_t));
    assert(pdu->ctrl_expander);
    pdu->ctrl_expander->dev_addr = CTRL_ADDR;
	max7314_init(pdu->ctrl_expander, pdu->hi2c);

	/* Same as shutdown */
	status = max7314_write_config(pdu->ctrl_expander, &config_data);
    if (status != HAL_OK) {
        serial_print("\n\rcntrl init fail\n\r");
	    free(pdu->shutdown_expander);
	    free(pdu);
	    return NULL;
    }

	/* Set global intensity */
	status = max7314_set_global_intensity(pdu->ctrl_expander, 0);
    if (status != HAL_OK) {
        serial_print("\n\rSet global intensity fail\n\r");
		free(pdu->ctrl_expander);
	    free(pdu->shutdown_expander);
	    free(pdu);
	    return NULL;
    }

    // set pins 15, 3, 2, 1, 0 to outputs
	uint8_t pin_config[2] = {0b11110000, 0b01111111};
    if (max7314_set_pin_modes(pdu->ctrl_expander, pin_config)) {
		serial_print("\n\rset pin modes fail\n\r");
        free(pdu->ctrl_expander);
        free(pdu->shutdown_expander);
        free(pdu);
        return NULL;
    }

	/* Create Mutex */
	pdu->mutex = osMutexNew(&pdu_mutex_attributes);
	assert(pdu->mutex);

	pdu->rtds_timer = osTimerNew(&rtds_shutoff_cb, osTimerOnce, pdu, NULL);

	//assert(max7314_set_pin_state(pdu->ctrl_expander, RTDS_CTRL, false));

	return pdu;
}

int8_t write_pump(pdu_t* pdu, bool status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, osWaitForever);
	if (stat) {
		osMutexRelease(pdu->mutex);
		return stat;
	}

	/* write pump over i2c */
    HAL_StatusTypeDef error = max7314_set_pin_state(pdu->ctrl_expander, PUMP_CTRL, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_fault(pdu_t* pdu, bool status)
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

int8_t sound_rtds(pdu_t* pdu) 
{
    if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;
	
	
	osTimerStart(pdu->rtds_timer, RTDS_DURATION);

	/* write RTDS over i2c */
    HAL_StatusTypeDef error = max7314_set_pin_state(pdu->ctrl_expander, RTDS_CTRL, false);
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

	HAL_StatusTypeDef error = max7314_read_pin(pdu->ctrl_expander, pin, status);
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
    HAL_StatusTypeDef error = max7314_read_pin(pdu->ctrl_expander, tsms_pin, status);
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
    HAL_StatusTypeDef error = max7314_read_pin(pdu->shutdown_expander, pin, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_rtds(pdu_t* pdu, bool status) 
{
    if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* write fan over i2c */
    HAL_StatusTypeDef error = max7314_set_pin_state(pdu->ctrl_expander, RTDS_CTRL, status);
    if(error != HAL_OK) {
        osMutexRelease(pdu->mutex);
        return error;
    }

	osMutexRelease(pdu->mutex);
	return 0;
}