#include "pdu.h"
#include "fault.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static osMutexAttr_t pdu_mutex_attributes;
extern I2C_HandleTypeDef hi2c2;

/* Wrappers for PCA9539 (GPIO Expander) */
static inline uint8_t pca_i2c_write(uint16_t dev_address, uint8_t reg,
				    uint8_t *data, uint8_t length)
{
	return HAL_I2C_Mem_Write(&hi2c2, dev_address, reg, I2C_MEMADD_SIZE_8BIT,
				 data, length, HAL_MAX_DELAY);
}
static inline uint8_t pca_i2c_read(uint16_t dev_address, uint8_t reg,
				   uint8_t *data, uint8_t length)
{
	return HAL_I2C_Mem_Read(&hi2c2, dev_address, reg, I2C_MEMADD_SIZE_8BIT,
				data, length, HAL_MAX_DELAY);
}

/* Wrappers for Current Sensor Read & Write */
static inline int ina_read_reg(uint16_t dev_addr, uint16_t reg, uint16_t *data)
{
	uint8_t buff[2];
	HAL_StatusTypeDef status;

	status = HAL_I2C_Mem_Read(&hi2c2, dev_addr, reg, I2C_MEMADD_SIZE_8BIT,
				  buff, 2, HAL_MAX_DELAY);
	if (status != HAL_OK) {
		return -1;
	}

	*data = (buff[0] << 8) | buff[1];
	return 0;
}
static inline int ina_write_reg(uint16_t dev_addr, uint16_t reg, uint16_t *data)
{
	uint8_t buff[2];
	buff[0] = (*data >> 8) & 0xFF;
	buff[1] = *data & 0xFF;

	HAL_StatusTypeDef status;
	status = HAL_I2C_Mem_Write(&hi2c2, dev_addr, reg, I2C_MEMADD_SIZE_8BIT,
				   buff, 2, HAL_MAX_DELAY);
	if (status != HAL_OK) {
		return -1;
	}

	return 0;
}

pdu_t *init_pdu(I2C_HandleTypeDef *hi2c, ADC_HandleTypeDef *pump_sensors_adc)
{
	pdu_t *pdu = malloc(sizeof(pdu_t));
	assert(pdu);
	pdu->hi2c = hi2c;
	pdu->pump_sensors_adc = pump_sensors_adc;
	assert(!HAL_ADC_Start_DMA(
		pdu->pump_sensors_adc, pdu->pump_sensors_dma_buf,
		sizeof(pdu->pump_sensors_dma_buf) / sizeof(uint32_t)));

	// FOR ALL 4 CURRENT SENSORS: Callibration constants taken from Altium on 11/6/24
	/* Initialize Motor Controller Current Sensor */
	pdu->motor_controller_current_sensor = malloc(sizeof(ina226_t));
	assert(pdu->motor_controller_current_sensor);
	ina226_init(pdu->motor_controller_current_sensor, ina_write_reg,
		    ina_read_reg, MOTOR_CONTROLLER_CURRENT_SENSOR_ADDR);
	int stat1 = ina226_calibrate(pdu->motor_controller_current_sensor,
				     0.01f, 3.0f);
	if (stat1 != 0) {
		printf("\n\rMotor Controller Current Sensor Init - Fail\n\r");
		free(pdu->motor_controller_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("\n\rMotor Controller Current Sensor Init - Success\n\r");

	/* Initialize Battbox Fans Current Sensor */
	pdu->battbox_fans_current_sensor = malloc(sizeof(ina226_t));
	assert(pdu->battbox_fans_current_sensor);
	ina226_init(pdu->battbox_fans_current_sensor, ina_write_reg,
		    ina_read_reg, BATTBOX_FANS_CURRENT_SENSOR_ADDR);
	int stat2 =
		ina226_calibrate(pdu->battbox_fans_current_sensor, 0.01f, 5.0f);
	if (stat2 != 0) {
		printf("\n\rBattbox Fans Current Sensor Init - Fail\n\r");
		free(pdu->battbox_fans_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("Battbox Fans Current Sensor Init - Success\n\r");

	/* Initialize Pumps Current Sensor */
	pdu->pumps_current_sensor = malloc(sizeof(ina226_t));
	assert(pdu->pumps_current_sensor);
	ina226_init(pdu->pumps_current_sensor, ina_write_reg, ina_read_reg,
		    PUMPS_CURRENT_SENSOR_ADDR);
	int stat3 = ina226_calibrate(pdu->pumps_current_sensor, 0.01f, 2.0f);
	if (stat3 != 0) {
		printf("\n\rPumps Current Sensor Init - Fail\n\r");
		free(pdu->pumps_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("Pumps Current Sensor Init - Success\n\r");

	/* Initialize LV Boards Current Sensor */
	pdu->lv_boards_current_sensor = malloc(sizeof(ina226_t));
	assert(pdu->lv_boards_current_sensor);
	ina226_init(pdu->lv_boards_current_sensor, ina_write_reg, ina_read_reg,
		    LV_BOARDS_CURRENT_SENSOR_ADDR);
	int stat4 =
		ina226_calibrate(pdu->lv_boards_current_sensor, 0.01f, 1.25f);
	if (stat4 != 0) {
		printf("\n\rLV Boards Current Sensor Init - Fail\n\r");
		free(pdu->lv_boards_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("LV Boards Current Sensor Init - Success\n\r");

	/* (Debug) Read All Current Sensors - Bus Voltage */
	float mc_volt;
	int stat5 = ina226_read_bus_voltage(
		pdu->motor_controller_current_sensor, &mc_volt);
	if (stat5 != 0) {
		printf("\n\rMotor Controller Current Sensor Read - Fail\n\r");
		free(pdu->motor_controller_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("Motor Controller Bus Voltage: %f\n\r", mc_volt);
	float bbf_voltage;
	int stat6 = ina226_read_bus_voltage(pdu->battbox_fans_current_sensor,
					    &bbf_voltage);
	if (stat6 != 0) {
		printf("\n\rBattbox Fans Current Sensor Read - Fail\n\r");
		free(pdu->battbox_fans_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("Battbox Fans Bus Voltage: %f\n\r", bbf_voltage);
	float pumps_voltage;
	int stat7 = ina226_read_bus_voltage(pdu->pumps_current_sensor,
					    &pumps_voltage);
	if (stat7 != 0) {
		printf("\n\rPumps Current Sensor Read - Fail\n\r");
		free(pdu->pumps_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("Pumps Bus Voltage: %f\n\r", pumps_voltage);
	float lv_voltage;
	int stat8 = ina226_read_bus_voltage(pdu->lv_boards_current_sensor,
					    &lv_voltage);
	if (stat8 != 0) {
		printf("\n\rLV Boards Current Sensor Read - Fail\n\r");
		free(pdu->lv_boards_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("LV Boards Bus Voltage: %f\n\r", lv_voltage);

	printf("\n\r");

	/* (Debug) Read All Current Sensors - Shunt Voltage */
	float mc_volt2;
	int stat9 = ina226_read_shunt_voltage(
		pdu->motor_controller_current_sensor, &mc_volt2);
	if (stat9 != 0) {
		printf("\n\rMotor Controller Current Sensor Read - Fail\n\r");
		free(pdu->motor_controller_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("Motor Controller Shunt Voltage: %f\n\r", mc_volt2);
	float bbf_voltage2;
	int stat10 = ina226_read_shunt_voltage(pdu->battbox_fans_current_sensor,
					       &bbf_voltage2);
	if (stat10 != 0) {
		printf("\n\rBattbox Fans Current Sensor Read - Fail\n\r");
		free(pdu->battbox_fans_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("Battbox Fans Shunt Voltage: %f\n\r", bbf_voltage2);
	float pumps_voltage2;
	int stat11 = ina226_read_shunt_voltage(pdu->pumps_current_sensor,
					       &pumps_voltage2);
	if (stat11 != 0) {
		printf("\n\rPumps Current Sensor Read - Fail\n\r");
		free(pdu->pumps_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("Pumps Shunt Voltage: %f\n\r", pumps_voltage2);
	float lv_voltage2;
	int stat12 = ina226_read_shunt_voltage(pdu->lv_boards_current_sensor,
					       &lv_voltage2);
	if (stat12 != 0) {
		printf("\n\rLV Boards Current Sensor Read - Fail\n\r");
		free(pdu->lv_boards_current_sensor);
		free(pdu);
		return NULL;
	}
	printf("LV Boards Shunt Voltage: %f\n\r", lv_voltage2);

	/* Initialize Shutdown GPIO Expander */
	pdu->shutdown_expander = malloc(sizeof(pca9539_t));
	assert(pdu->shutdown_expander);
	pca9539_init(pdu->shutdown_expander, pca_i2c_write, pca_i2c_read,
		     SHUTDOWN_ADDR);

	// all shutdown expander things are inputs
	uint8_t shutdown_config_directions = 0b00000000;
	HAL_StatusTypeDef status =
		pca9539_write_reg(pdu->shutdown_expander, PCA_DIRECTION_0_REG,
				  shutdown_config_directions);
	if (status != HAL_OK) {
		printf("\n\rshutdown write fail\n\r");
		free(pdu->shutdown_expander);
		free(pdu);
		return NULL;
	}
	status = pca9539_write_reg(pdu->shutdown_expander, PCA_DIRECTION_1_REG,
				   shutdown_config_directions);
	if (status != HAL_OK) {
		printf("\n\rshutdown wrtie 2 fail\n\r");
		free(pdu->shutdown_expander);
		free(pdu);
		return NULL;
	}

	/* Initialize Control GPIO Expander */
	pdu->ctrl_expander = malloc(sizeof(pca9539_t));
	assert(pdu->ctrl_expander);
	pca9539_init(pdu->ctrl_expander, pca_i2c_write, pca_i2c_read,
		     CTRL_ADDR);

	// write everything OFF, FAULT 1 is off
	uint8_t buf = 0b00000010;
	pca9539_write_reg(pdu->ctrl_expander, PCA_OUTPUT_0_REG, buf);
	pca9539_write_reg(pdu->ctrl_expander, PCA_OUTPUT_1_REG, buf);

	// pin 0 to the right
	buf = 0b11110000;
	status =
		pca9539_write_reg(pdu->ctrl_expander, PCA_DIRECTION_0_REG, buf);
	if (status != HAL_OK) {
		printf("cntrl init fail\n");
		free(pdu->ctrl_expander);
		free(pdu);
		return NULL;
	}
	// pin 0 to the right
	buf = 0b01111111;
	status =
		pca9539_write_reg(pdu->ctrl_expander, PCA_DIRECTION_1_REG, buf);
	if (status != HAL_OK) {
		printf("cntrl init fail\n");
		free(pdu->ctrl_expander);
		free(pdu);
		return NULL;
	}

	/* Create Mutex */
	pdu->mutex = osMutexNew(&pdu_mutex_attributes);
	assert(pdu->mutex);

	return pdu;
}

osThreadId_t rtds_thread;
const osThreadAttr_t rtds_attributes = { .name = "RtdsThread",
					 .stack_size = 512,
					 /* The task will run infrequently */
					 .priority = osPriorityRealtime7 };

void vRTDS(void *arg)
{
	pdu_t *pdu = (pdu_t *)arg;
	assert(pdu);

	fault_data_t rtds_fault = { .id = RTDS_FAULT, .severity = DEFCON4 };

	for (;;) {
		osThreadFlagsWait(SOUND_RTDS_FLAG, osFlagsWaitAny,
				  osWaitForever);
		if (write_rtds(pdu, true)) {
			rtds_fault.diag = "Unable to sound RTDS";
			queue_fault(&rtds_fault);
		}
		osDelay(RTDS_DURATION);
		if (write_rtds(pdu, false)) {
			rtds_fault.diag = "Unable to stop RTDS";
			queue_fault(&rtds_fault);
		}
	}
}

/* CTRL Line Functions */
static int8_t write_ctrl(pdu_t *pdu, bool state, uint8_t pin, uint8_t reg)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	HAL_StatusTypeDef error =
		pca9539_write_pin(pdu->ctrl_expander, reg, pin, state);

	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_pump_0(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_PUMP_CTRL0, PCA_OUTPUT_0_REG);
}

int8_t write_pump_1(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_PUMP_CTRL1, PCA_OUTPUT_0_REG);
}

int8_t write_24V_12V_buck(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_BUCK_CTRL, PCA_OUTPUT_0_REG);
}

int8_t write_brakelight(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_BRKLIGHT_CTRL, PCA_OUTPUT_0_REG);
}

int8_t write_fan_battbox(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_FANBATTBOX_CTRL, PCA_OUTPUT_0_REG);
}

int8_t write_rtds(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_RTD_CTRL, PCA_OUTPUT_1_REG);
}

int8_t write_radfan_0(pdu_t *pdu, bool state)
{
	return -1; // Replace with actual stuff when PDU Radfan CTRL is added to board
}

int8_t write_radfan_1(pdu_t *pdu, bool state)
{
	return -1; // Replace with actual stuff when PDU Radfan CTRL is added to board
}

/* Read Pump Sensors ADC DMA */
void read_pump_sensors(pdu_t *pdu, uint32_t pump_sensors_buf[2])
{
	memcpy(pump_sensors_buf, &pdu->pump_sensors_dma_buf,
	       sizeof(pdu->pump_sensors_dma_buf));
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

	status[PUMP_FUSE_STAT0] = bank0[PIN_PUMP_FUSE_STAT0];
	status[SD_TO_BRB_FUSE] = bank0[PIN_SD_TO_BRB_FUSE_STAT];
	status[LV_BOARDS_FUSE_STAT] = bank1[PIN_LV_BOARDS_FUSE_STAT];
	status[RADFAN_FUSE_STAT] = bank1[PIN_RADFAN_FUSE_STAT];
	status[BATTBOX_FUSE_STAT] = bank1[PIN_BATTBOX_FUSE_STAT];
	status[BUCK_FUSE_STAT] = bank1[PIN_BUCK_FUSE_STAT];
	status[FANBATTBOX_STAT] = bank1[PIN_FANBATTBOX_STAT];
	status[PUMP_FUSE_STAT1] = bank1[PIN_PUMP_FUSE_STAT1];
	status[DASHBOARD_FUSE_STAT] = bank1[PIN_DASHBOARD_FUSE_STAT];
	status[BRKLIGHT_FUSE_STAT] = bank1[PIN_BRKLIGHT_FUSE_STAT];

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
	uint8_t config = 0;
	HAL_StatusTypeDef error = pca9539_read_pin(pdu->shutdown_expander,
						   PCA_INPUT_1_REG,
						   PIN_TMS_SENSE, &config);
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

	status[HVD_GOOD] = bank0[PIN_HVD_GOOD];
	status[HVC_GOOD] = bank0[PIN_HVC_GOOD];
	status[BOTS_GOOD] = bank0[PIN_BOTS_GOOD];
	status[CKPT_BRB] = bank0[PIN_CKPT_BRB];
	status[BMS_GOOD] = bank0[PIN_BMS_GOOD];
	status[INERTIA_SW_GOOD] = bank0[PIN_INERTIA_SW_GOOD];
	status[SPARE_GPIO0] = bank0[PIN_SPARE_GPIO0];
	status[IMD_GOOD] = bank0[PIN_IMD_GOOD];

	status[BSPD_GOOD] = bank1[PIN_BSPD_GOOD];

	osMutexRelease(pdu->mutex);
	return 0;
}

static int8_t read_current(pdu_t *pdu, ina226_t *ina, float *data)
{
	if (!pdu || !ina || !data)
		return -1;

	float current;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	int status = ina226_read_current(ina, &current);

	if (status != 0) {
		osMutexRelease(pdu->mutex);
		return status;
	}

	*data = current;
	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t read_all_current(pdu_t *pdu, float *motor_controller_current,
			float *battbox_fans_current, float *pumps_current,
			float *lv_boards_current)
{
	if (!read_current(pdu, pdu->motor_controller_current_sensor,
			  motor_controller_current))
		return -1;
	if (!read_current(pdu, pdu->battbox_fans_current_sensor,
			  battbox_fans_current))
		return -1;
	if (!read_current(pdu, pdu->pumps_current_sensor,
			  motor_controller_current))
		return -1;
	if (!read_current(pdu, pdu->lv_boards_current_sensor,
			  battbox_fans_current))
		return -1;
	return 0;
}

int8_t read_brake_state(pdu_t *pdu, bool *status)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	/* read pin over i2c */
	uint8_t config = 0;
	HAL_StatusTypeDef error = pca9539_read_pin(pdu->ctrl_expander,
						   PCA_INPUT_1_REG,
						   PIN_BRKLIGHT_CTRL, &config);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}
	*status = config;

	osMutexRelease(pdu->mutex);
	return 0;
}