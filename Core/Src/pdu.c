#include "pdu.h"
#include "serial_monitor.h"
#include <assert.h>
#include <stdio.h>
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

#define SHUTDOWN_ADDR PCA_I2C_ADDR_0
#define CTRL_ADDR	  PCA_I2C_ADDR_1
#define RTDS_DURATION 2500

#define PIN_MODE_OUTPUT 0
#define PIN_MODE_INPUT	1

static osMutexAttr_t pdu_mutex_attributes;

static void rtds_shutoff_cb(void* pv_params)
{
	pdu_t* pdu		= (pdu_t*)pv_params;
	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return;

	/* write RTDS over i2c */
	HAL_StatusTypeDef error = pca9539_write_pin(pdu->ctrl_expander, PCA_OUTPUT, PCA_B1, RTDS_CTRL, false);
	if (error != HAL_OK) {
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
	pdu->shutdown_expander = malloc(sizeof(pca9539_t));
	assert(pdu->shutdown_expander);
	pca9539_init(pdu->shutdown_expander, pdu->hi2c, SHUTDOWN_ADDR);

	// all shutdown expander things are inputs
	bool shutdown_config_directions[8] = { 0, 0, 0, 0, 0, 0, 0, 0 } ;
	HAL_StatusTypeDef status = pca9539_write_pins(pdu->shutdown_expander, PCA_DIRECTION, PCA_B0, shutdown_config_directions);
	if (status != HAL_OK) {
		printf("\n\rshutdown init fail\n\r");
		free(pdu->shutdown_expander);
		free(pdu);
		return NULL;
	}
	status
		= pca9539_write_pins(pdu->shutdown_expander, PCA_DIRECTION, PCA_B1, shutdown_config_directions);
	if (status != HAL_OK) {
		printf("\n\rshutdown init fail\n\r");
		free(pdu->shutdown_expander);
		free(pdu);
		return NULL;
	}

	/* Initialize Control GPIO Expander */
	pdu->ctrl_expander = malloc(sizeof(pca9539_t));
	assert(pdu->ctrl_expander);
	pca9539_init(pdu->ctrl_expander, pdu->hi2c, CTRL_ADDR);


	bool control_config_directions_0[8] = { 1, 1, 1, 1, 0, 0, 0, 0 };
	status = pca9539_write_pins(pdu->ctrl_expander, PCA_DIRECTION, PCA_B0, control_config_directions_0);
	if (status != HAL_OK) {
		printf("\n\rcntrl init fail\n\r");
		free(pdu->ctrl_expander);
		free(pdu);
		return NULL;
	}
	bool control_config_directions_1[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };
	status = pca9539_write_pins(pdu->ctrl_expander, PCA_DIRECTION, PCA_B1, control_config_directions_1);
	if (status != HAL_OK) {
		printf("\n\rcntrl init fail\n\r");
		free(pdu->ctrl_expander);
		free(pdu);
		return NULL;
	}

	/* Create Mutex */
	pdu->mutex = osMutexNew(&pdu_mutex_attributes);
	assert(pdu->mutex);

	pdu->rtds_timer = osTimerNew(&rtds_shutoff_cb, osTimerOnce, pdu, NULL);

	//assert(!max7314_set_pin_state(pdu->ctrl_expander, RTDS_CTRL, false));

	// DEBUG To test RTDS
	// sound_rtds(pdu);

	// DEBUG brakelight
	// write_brakelight(pdu, true);

	// DEBUG pump
	// write_pump(pdu, false);

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
	HAL_StatusTypeDef error = pca9539_write_pin(pdu->ctrl_expander, PCA_OUTPUT, PCA_B0, PUMP_CTRL, status);
	if (error != HAL_OK) {
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
	HAL_StatusTypeDef error = pca9539_write_pin(pdu->ctrl_expander, PCA_OUTPUT, PCA_B0, RADFAN_CTRL, status);
	if (error != HAL_OK) {
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
	HAL_StatusTypeDef error = pca9539_write_pin(pdu->ctrl_expander, PCA_OUTPUT, PCA_B0, BRKLIGHT_CTRL, status);
	if (error != HAL_OK) {
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
	HAL_StatusTypeDef error = pca9539_write_pin(pdu->ctrl_expander, PCA_OUTPUT, PCA_B0, BATBOXFAN_CTRL, status);
	if (error != HAL_OK) {
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
	HAL_StatusTypeDef error = pca9539_write_pin(pdu->ctrl_expander, PCA_OUTPUT, PCA_B1, RTDS_CTRL, true);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);

	return 0;
}

int8_t read_fuses(pdu_t* pdu, bool status[MAX_FUSES])
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	bool bank0[8];
	HAL_StatusTypeDef error = pca9539_read_pins(pdu->ctrl_expander, PCA_INPUT, PCA_B0, bank0);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	bool bank1[8];
	error = pca9539_read_pins(pdu->ctrl_expander, PCA_INPUT, PCA_B1, bank1);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	status[FUSE_BATTBOX] = bank0[4];
	status[FUSE_LVBOX] = bank0[5];
	status[FUSE_FAN_RADIATOR] = bank0[6];
	status[FUSE_MC] = bank0[7];
	status[FUSE_FAN_BATTBOX] = bank1[0];
	status[FUSE_PUMP] = bank1[1];
	status[FUSE_DASHBOARD] = bank1[2];
	status[FUSE_BRAKELIGHT] = bank1[3];
	status[FUSE_BRB] = bank1[4];

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
	uint8_t tsms_pin		= 14;
	HAL_StatusTypeDef error = pca9539_read_pin(pdu->ctrl_expander, PCA_INPUT, PCA_B1, tsms_pin, status);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t read_shutdown(pdu_t* pdu, bool status[MAX_SHUTDOWN_STAGES])
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	bool bank0[8];
	HAL_StatusTypeDef error = pca9539_read_pins(pdu->shutdown_expander, PCA_INPUT, PCA_B0, bank0);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	bool bank1[8];
	error = pca9539_read_pins(pdu->shutdown_expander, PCA_INPUT, PCA_B1, bank1);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	status[CKPT_BRB_CLR] = bank0[0];
	status[BMS_OK] = bank0[2];
	status[INERTIA_SW_OK] = bank0[3];
	status[SPARE_GPIO1_OK] = bank0[4];
	status[IMD_OK] = bank0[5];
	status[BSPD_OK] = bank1[0];
	status[BOTS_OK] = bank1[5];
	status[HVD_INTLK_OK] = bank1[6];
	status[HVC_INTLK_OK] = bank1[7];

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
	HAL_StatusTypeDef error = pca9539_write_pin(pdu->ctrl_expander, PCA_OUTPUT, PCA_B1, RTDS_CTRL, status);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);
	return 0;
}