#include "pdu.h"
#include "serial_monitor.h"
#include "fault.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define PUMP_CTRL      0
#define RADFAN_CTRL    1
#define BRKLIGHT_CTRL  2
#define BATBOXFAN_CTRL 3
#define RTDS_CTRL      7 // PORT 17 BANK 1 (so read with 1_REG)
// #define TSMS_CTRL	   0x04
// #define SMBALERT	   0x05
#define MUTEX_TIMEOUT osWaitForever /* ms */

#define SHUTDOWN_ADDR PCA_I2C_ADDR_3
#define CTRL_ADDR     PCA_I2C_ADDR_2
#define RTDS_DURATION 1750 /* ms at 1kHz tick rate */

static osMutexAttr_t pdu_mutex_attributes;

static uint8_t sound_rtds(pdu_t *pdu)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* write RTDS over i2c */
	HAL_StatusTypeDef error = pca9539_write_pin(
		pdu->ctrl_expander, PCA_OUTPUT_1_REG, RTDS_CTRL, true);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);

	return 0;
}

static uint8_t rtds_shutoff(void *pv_params)
{
	pdu_t *pdu = (pdu_t *)pv_params;
	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* write RTDS over i2c */
	HAL_StatusTypeDef error = pca9539_write_pin(
		pdu->ctrl_expander, PCA_OUTPUT_1_REG, RTDS_CTRL, false);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);
	return 0;
}

osThreadId_t rtds_thread;
const osThreadAttr_t rtds_attributes = { .name = "RtdsThread",
					 .stack_size = 512,
					 /* The task will run infrequently */
					 .priority = osPriorityRealtime7 };

void vRTDS(void *arg)
{
	pdu_t *pdu = (pdu_t *)arg;

	fault_data_t rtds_fault = { .id = RTDS_FAULT, .severity = DEFCON4 };

	for (;;) {
		osThreadFlagsWait(SOUND_RTDS_FLAG, osFlagsWaitAny,
				  osWaitForever);
		if (sound_rtds(pdu)) {
			rtds_fault.diag = "Unable to sound RTDS";
			queue_fault(&rtds_fault);
		}
		osDelay(RTDS_DURATION);
		if (rtds_shutoff(pdu)) {
			rtds_fault.diag = "Unable to stop RTDS";
			queue_fault(&rtds_fault);
		}
	}
}

pdu_t *init_pdu(I2C_HandleTypeDef *hi2c)
{
	assert(hi2c);

	/* Create PDU struct */
	pdu_t *pdu = malloc(sizeof(pdu_t));
	assert(pdu);

	pdu->hi2c = hi2c;

	/* Initialize Shutdown GPIO Expander */
	pdu->shutdown_expander = malloc(sizeof(pca9539_t));
	assert(pdu->shutdown_expander);
	// pca9539_init(pdu->shutdown_expander, pdu->hi2c, SHUTDOWN_ADDR);
	// if (status != HAL_OK) {
	// 	printf("\n\rshutdown init fail\n\r");
	// 	free(pdu->shutdown_expander);
	// 	free(pdu);
	// 	return NULL;
	// }

	// all shutdown expander things are inputs
	// uint8_t shutdown_config_directions = 0b00000000;
	//  HAL_StatusTypeDef status = pca9539_write_reg(pdu->shutdown_expander, PCA_DIRECTION_0_REG,
	//  shutdown_config_directions);
	// if (status != HAL_OK) {
	// 	printf("\n\rshutdown write fail\n\r");
	// 	free(pdu->shutdown_expander);
	// 	free(pdu);
	// 	return NULL;
	// }
	// status
	// 	= pca9539_write_reg(pdu->shutdown_expander, PCA_DIRECTION_1_REG,
	// shutdown_config_directions); if (status != HAL_OK) { 	printf("\n\rshutdown wrtie 2 fail\n\r");
	// 	free(pdu->shutdown_expander);
	// 	free(pdu);
	// 	return NULL;
	// }

	/* Initialize Control GPIO Expander */
	pdu->ctrl_expander = malloc(sizeof(pca9539_t));
	assert(pdu->ctrl_expander);
	pca9539_init(pdu->ctrl_expander, pdu->hi2c, CTRL_ADDR);

	// write everything OFF, FAULT 1 is off
	uint8_t buf = 0b00000010;
	pca9539_write_reg(pdu->ctrl_expander, PCA_OUTPUT_0_REG, buf);
	pca9539_write_reg(pdu->ctrl_expander, PCA_OUTPUT_1_REG, buf);

	// pin 0 to the right
	buf = 0b11110000;
	HAL_StatusTypeDef status =
		pca9539_write_reg(pdu->ctrl_expander, PCA_DIRECTION_0_REG, buf);
	if (status != HAL_OK) {
		printf("\n\rcntrl init fail\n\r");
		free(pdu->ctrl_expander);
		free(pdu);
		return NULL;
	}
	// pin 0 to the right
	buf = 0b01111111;
	status =
		pca9539_write_reg(pdu->ctrl_expander, PCA_DIRECTION_1_REG, buf);
	if (status != HAL_OK) {
		printf("\n\rcntrl init fail\n\r");
		free(pdu->ctrl_expander);
		free(pdu);
		return NULL;
	}

	/* Create Mutex */
	pdu->mutex = osMutexNew(&pdu_mutex_attributes);
	assert(pdu->mutex);

	return pdu;
}

int8_t write_pump(pdu_t *pdu, bool status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, osWaitForever);
	if (stat) {
		osMutexRelease(pdu->mutex);
		return stat;
	}

	/* write pump over i2c */
	HAL_StatusTypeDef error = pca9539_write_pin(
		pdu->ctrl_expander, PCA_OUTPUT_0_REG, PUMP_CTRL, status);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_fault(pdu_t *pdu, bool status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* write fault GPIO over i2c, fault line is inverted */
	HAL_StatusTypeDef error = pca9539_write_pin(
		pdu->ctrl_expander, PCA_OUTPUT_0_REG, RADFAN_CTRL, !status);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_brakelight(pdu_t *pdu, bool status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* write brakelight over i2c */
	HAL_StatusTypeDef error = pca9539_write_pin(
		pdu->ctrl_expander, PCA_OUTPUT_0_REG, BRKLIGHT_CTRL, status);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_fan_battbox(pdu_t *pdu, bool status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* write fan over i2c */
	HAL_StatusTypeDef error = pca9539_write_pin(
		pdu->ctrl_expander, PCA_OUTPUT_0_REG, 2, status);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);
	return 0;
}

static void deconstruct_buf(uint8_t data, bool config[8])
{
	for (uint8_t i = 0; i < 8; i++) {
		config[i] = (data >> i) & 1;
	}
}

int8_t read_fuses(pdu_t *pdu, bool status[MAX_FUSES])
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	uint8_t bank0_d = 0;
	HAL_StatusTypeDef error =
		pca9539_read_reg(pdu->ctrl_expander, PCA_INPUT_0_REG, &bank0_d);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	uint8_t bank1_d = 0;
	error = pca9539_read_reg(pdu->ctrl_expander, PCA_INPUT_1_REG, &bank1_d);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	bool bank0[8];
	deconstruct_buf(bank0_d, bank0);

	bool bank1[8];
	deconstruct_buf(bank1_d, bank1);

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

int8_t read_tsms_sense(pdu_t *pdu, bool *status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* read pin over i2c */
	uint8_t tsms_pin = 6;
	uint8_t config = 0;
	HAL_StatusTypeDef error = pca9539_read_pin(
		pdu->ctrl_expander, PCA_INPUT_1_REG, tsms_pin, &config);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}
	*status = config;

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t read_shutdown(pdu_t *pdu, bool status[MAX_SHUTDOWN_STAGES])
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	uint8_t bank0_d = 0;
	HAL_StatusTypeDef error = pca9539_read_reg(pdu->shutdown_expander,
						   PCA_INPUT_0_REG, &bank0_d);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	uint8_t bank1_d = 0;
	error = pca9539_read_reg(pdu->shutdown_expander, PCA_INPUT_1_REG,
				 &bank1_d);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	bool bank0[8];
	deconstruct_buf(bank0_d, bank0);

	bool bank1[8];
	deconstruct_buf(bank1_d, bank1);

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
