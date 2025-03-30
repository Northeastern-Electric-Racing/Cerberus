#include "pdu.h"
#include "fault.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static osMutexAttr_t pdu_mutex_attributes;
extern I2C_HandleTypeDef hi2c2;

/* Wrappers for TCA9539 (GPIO Expander) */
static inline uint8_t tca_i2c_write(uint16_t dev_address, uint8_t reg,
				    uint8_t *data, uint8_t length)
{
	return HAL_I2C_Mem_Write(&hi2c2, dev_address, reg, I2C_MEMADD_SIZE_8BIT,
				 data, length, HAL_MAX_DELAY);
}
static inline uint8_t tca_i2c_read(uint16_t dev_address, uint8_t reg,
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

int init_ina(pdu_t *pdu, ina226_t *ina, uint16_t dev_addr, float r_shunt,
	     float max_current)
{
	ina226_init(ina, ina_write_reg, ina_read_reg, dev_addr);
	int stat = ina226_calibrate(ina, r_shunt, max_current);
	if (stat != 0) {
		printf("Current Sensor Init Failed - (ID: %X)\n", dev_addr);
		free(ina);
		free(pdu);
		return -1;
	}
	printf("Current Sensor Init Success - (ID: %X)\n", dev_addr);
	return 0;
}

pdu_t *init_pdu(I2C_HandleTypeDef *hi2c, ADC_HandleTypeDef *pump_sensors_adc)
{
	pdu_t *pdu = malloc(sizeof(pdu_t));
	assert(pdu);
	pdu->hi2c = hi2c;
	pdu->pump_sensors_adc = pump_sensors_adc;
	assert(!HAL_ADC_Start_DMA(
		pdu->pump_sensors_adc, (uint32_t *)pdu->pump_sensors_dma_buf,
		sizeof(pdu->pump_sensors_dma_buf) / sizeof(uint16_t)));

	/* Reset GPIO Expanders Before Init */
	HAL_GPIO_WritePin(GPIOC, CTRL_RESET_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, SHUTDOWN_RESET_PIN, GPIO_PIN_RESET);
	osDelay(1);
	HAL_GPIO_WritePin(GPIOC, CTRL_RESET_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, SHUTDOWN_RESET_PIN, GPIO_PIN_SET);
	osDelay(1);
	// FOR 3 CURRENT SENSORS: Callibration constants taken from Altium on 11/6/24
	// Batt Box Fan current sensors removed due to hardware issues
	/* Initialize Motor Controller Current Sensor */
	pdu->motor_controller_current_sensor = malloc(sizeof(ina226_t));
	if (init_ina(pdu, pdu->motor_controller_current_sensor,
		     MOTOR_CONTROLLER_CURRENT_SENSOR_ADDR, 0.01f, 3.0f)) {
		return NULL;
	}

	/* Initialize Pumps Current Sensor */
	pdu->pumps_current_sensor = malloc(sizeof(ina226_t));
	if (init_ina(pdu, pdu->pumps_current_sensor, PUMPS_CURRENT_SENSOR_ADDR,
		     0.01f, 2.0f)) {
		return NULL;
	}

	/* Initialize LV Boards Current Sensor */
	pdu->lv_boards_current_sensor = malloc(sizeof(ina226_t));
	if (init_ina(pdu, pdu->lv_boards_current_sensor,
		     LV_BOARDS_CURRENT_SENSOR_ADDR, 0.01f, 1.25f)) {
		return NULL;
	}

	/* Initialize Shutdown GPIO Expander */
	pdu->shutdown_expander = malloc(sizeof(tca9539_t));
	assert(pdu->shutdown_expander);
	tca9539_init(pdu->shutdown_expander, tca_i2c_write, tca_i2c_read,
		     SHUTDOWN_ADDR);

	/* Configure Shutdown Expander - Bank 0 */
	uint8_t shutdown_config_bank0 = 0b11111111;
	HAL_StatusTypeDef status = tca9539_write_reg(pdu->shutdown_expander,
						     TCA_CONFIGURATION_PORT_0,
						     shutdown_config_bank0);
	if (status != HAL_OK) {
		printf("\n\rShutdown config fail - Bank 0\n\r");
		free(pdu->shutdown_expander);
		free(pdu);
		return NULL;
	}

	/* Configure Shutdown Expander - Bank 1 */
	uint8_t shutdown_config_bank1 = 0b11111111;
	status = tca9539_write_reg(pdu->shutdown_expander,
				   TCA_CONFIGURATION_PORT_1,
				   shutdown_config_bank1);
	if (status != HAL_OK) {
		printf("\n\rShutdown config fail - Bank 1\n\r");
		free(pdu->shutdown_expander);
		free(pdu);
		return NULL;
	}

	/* Initialize Control GPIO Expander */
	pdu->ctrl_expander = malloc(sizeof(tca9539_t));
	assert(pdu->ctrl_expander);
	tca9539_init(pdu->ctrl_expander, tca_i2c_write, tca_i2c_read,
		     CTRL_ADDR);

	/* Initialize all outputs to 0 */
	uint8_t ctrl_output_bank0 = 0b00000000;
	tca9539_write_reg(pdu->ctrl_expander, TCA_OUTPUT_PORT_0,
			  ctrl_output_bank0);
	uint8_t ctrl_output_bank1 = 0b00000000;
	tca9539_write_reg(pdu->ctrl_expander, TCA_OUTPUT_PORT_1,
			  ctrl_output_bank1);

	/* Configure Control Expander - Bank 0 */
	uint8_t ctrl_config_bank0 = 0b00000001;
	status = tca9539_write_reg(pdu->ctrl_expander, TCA_CONFIGURATION_PORT_0,
				   ctrl_config_bank0);
	if (status != HAL_OK) {
		printf("CTRL config fail - Bank 0\n");
		free(pdu->ctrl_expander);
		free(pdu);
		return NULL;
	}

	/* Configure Control Expander - Bank 1 */
	uint8_t ctrl_config_bank1 = 0b11111111;
	status = tca9539_write_reg(pdu->ctrl_expander, TCA_CONFIGURATION_PORT_1,
				   ctrl_config_bank1);
	if (status != HAL_OK) {
		printf("CTRL config fail - Bank 1\n");
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

	fault_data_t rtds_fault = {
		.fault_id = RTDS_FAULT,
	};

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
		tca9539_write_pin(pdu->ctrl_expander, reg, pin, state);

	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t write_pump_1(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_PUMP_CTRL_1, TCA_OUTPUT_PORT_0);
}

int8_t write_pump_2(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_PUMP_CTRL_2, TCA_OUTPUT_PORT_0);
}

int8_t write_brakelight(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_BRKLIGHT_CTRL, TCA_OUTPUT_PORT_0);
}

int8_t write_fan_battbox(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_FANBATTBOX_CTRL, TCA_OUTPUT_PORT_0);
}

int8_t write_rtds(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_RTDS_CTRL, TCA_OUTPUT_PORT_0);
}

int8_t write_radfan_1(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_RADFAN_CTRL_1, TCA_OUTPUT_PORT_0);
}

int8_t write_radfan_2(pdu_t *pdu, bool state)
{
	return write_ctrl(pdu, state, PIN_RADFAN_CTRL_2, TCA_OUTPUT_PORT_0);
}

/* Read Pump Sensors ADC DMA */
void read_pump_sensors(pdu_t *pdu, uint16_t pump_sensors_buf[2])
{
	memcpy(pump_sensors_buf, &pdu->pump_sensors_dma_buf,
	       sizeof(pdu->pump_sensors_dma_buf));
}

int8_t read_fuses(pdu_t *pdu, bitstream_t *bitstream)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	uint8_t bank0_d = 0;
	HAL_StatusTypeDef error = tca9539_read_reg(pdu->ctrl_expander,
						   TCA_INPUT_PORT_0, &bank0_d);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	uint8_t bank1_d = 0;
	error = tca9539_read_reg(pdu->ctrl_expander, TCA_INPUT_PORT_1,
				 &bank1_d);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	bitstream_t fuses;
	uint8_t fuse_data[2];
	bitstream_init(&fuses, fuse_data, 2);

	// clang-format off
	bitstream_add(&fuses, EXTRACT_BIT(bank0_d, PIN_BATTBOX_FUSE_STAT), 1);			// Read Pin P07
	bitstream_add(&fuses, EXTRACT_BIT(bank1_d, PIN_LV_BOARDS_FUSE_STAT), 1);		// Read Pin P10
	bitstream_add(&fuses, EXTRACT_BIT(bank1_d, PIN_RADFAN_FUSE_STAT), 1);			// Read Pin P11
	bitstream_add(&fuses, EXTRACT_BIT(bank1_d, PIN_FANBATTBOX_FUSE_STAT), 1);		// Read Pin P12
	bitstream_add(&fuses, EXTRACT_BIT(bank1_d, PIN_DASHBOARD_FUSE_STAT), 1);		// Read Pin P13
	bitstream_add(&fuses, EXTRACT_BIT(bank1_d, PIN_BRKLIGHT_FUSE_STAT), 1);			// Read Pin P14
	bitstream_add(&fuses, EXTRACT_BIT(bank1_d, PIN_SD_TO_BRB_FUSE_STAT), 1);		// Read Pin P15
	bitstream_add(&fuses, EXTRACT_BIT(bank1_d, PIN_PUMP_FUSE_STAT1), 1);			// Read Pin P16
	bitstream_add(&fuses, EXTRACT_BIT(bank1_d, PIN_PUMP_FUSE_STAT2), 1);			// Read Pin P17
	bitstream_add(&fuses, 0, 7); 													// Extra (7 bits)
	// clang-format on

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
	HAL_StatusTypeDef error = tca9539_read_pin(pdu->shutdown_expander,
						   TCA_INPUT_PORT_1,
						   PIN_TSMS_SENSE, &config);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}
	*status = config;

	osMutexRelease(pdu->mutex);
	return 0;
}

int8_t read_shutdown(pdu_t *pdu, bitstream_t *bitstream)
{
	if (!pdu)
		return -1;

	osStatus_t stat = osMutexAcquire(pdu->mutex, MUTEX_TIMEOUT);
	if (stat)
		return stat;

	uint8_t bank0_d = 0;
	HAL_StatusTypeDef error = tca9539_read_reg(pdu->shutdown_expander,
						   TCA_INPUT_PORT_0, &bank0_d);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}
	uint8_t bank1_d = 0;
	error = tca9539_read_reg(pdu->shutdown_expander, TCA_INPUT_PORT_1,
				 &bank1_d);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}

	// clang-format off
	bitstream_t shutdown;
	uint8_t shutdown_data[1];
	bitstream_init(&shutdown, shutdown_data, 1);

	bitstream_add(&shutdown, EXTRACT_BIT(bank0_d, PIN_BMS_GOOD), 1); 			// Read Pin P01
	bitstream_add(&shutdown, EXTRACT_BIT(bank0_d, PIN_INERTIA_SW_GOOD), 1); 	// Read Pin P02
	bitstream_add(&shutdown, EXTRACT_BIT(bank0_d, PIN_IMD_GOOD), 1); 			// Read Pin P04
	bitstream_add(&shutdown, EXTRACT_BIT(bank0_d, PIN_BSPD_GOOD), 1); 			// Read Pin P05
	bitstream_add(&shutdown, EXTRACT_BIT(bank1_d, PIN_MC_STAT), 1); 			// Read Pin P11
	bitstream_add(&shutdown, EXTRACT_BIT(bank1_d, PIN_BOTS_GOOD), 1); 			// Read Pin P15
	bitstream_add(&shutdown, EXTRACT_BIT(bank1_d, PIN_HVD_INTLK_GOOD), 1); 		// Read Pin P16
	bitstream_add(&shutdown, EXTRACT_BIT(bank1_d, PIN_HVC_INTLK_GOOD), 1); 		// Read Pin P17
	// clang-format on

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
	if (read_current(pdu, pdu->motor_controller_current_sensor,
			 motor_controller_current))
		return -1;
	if (read_current(pdu, pdu->pumps_current_sensor, pumps_current))
		return -1;
	if (read_current(pdu, pdu->lv_boards_current_sensor, lv_boards_current))
		return -1;

	// setting to 0 since current sensor is nonfunctional
	*battbox_fans_current = 0; 

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
	HAL_StatusTypeDef error = tca9539_read_pin(pdu->ctrl_expander,
						   TCA_INPUT_PORT_0,
						   PIN_BRKLIGHT_CTRL, &config);
	if (error != HAL_OK) {
		osMutexRelease(pdu->mutex);
		return error;
	}
	*status = config;

	osMutexRelease(pdu->mutex);
	return 0;
}
