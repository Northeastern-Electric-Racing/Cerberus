#ifndef PDU_H
#define PDU_H

#include <stdbool.h>
#include "cmsis_os.h"
#include "tca9539.h"
#include "INA226.h"
#include <stdbool.h>
#include <stdint.h>
#include "bitstream.h"
#include "c_utils.h"
#include "stm32f4xx_hal.h"

typedef struct {
	I2C_HandleTypeDef *hi2c;
	osMutexId_t *mutex;
	tca9539_t *shutdown_expander;
	tca9539_t *ctrl_expander;

	ina226_t *motor_controller_current_sensor;
	ina226_t *battbox_fans_current_sensor;
	ina226_t *pumps_current_sensor;
	ina226_t *lv_boards_current_sensor;

	ADC_HandleTypeDef *pump_sensors_adc;
	uint16_t pump_sensors_dma_buf[2];
} pdu_t;

/* Creates a new PDU interface */
pdu_t *init_pdu(I2C_HandleTypeDef *hi2c, ADC_HandleTypeDef *pump_sensors_adc);

/* Functions to Control PDU */
/// MOTOR PUMP
int8_t write_pump_1(pdu_t *pdu, bool state);
/// MC PUMP
int8_t write_pump_2(pdu_t *pdu, bool state);
int8_t write_brakelight(pdu_t *pdu, bool state);
int8_t write_fan_battbox(pdu_t *pdu, bool state);
int8_t write_rtds(pdu_t *pdu, bool state);
/// MC FAN
int8_t write_radfan_1(pdu_t *pdu, bool state);
/// MOTOR FAN
int8_t write_radfan_2(pdu_t *pdu, bool state);

/**
 * @brief Read the status of the PDU fuses.
 * 
 * @param pdu Pointer to struct representing the PDU
 * @param status Bitstream for storing fuse data
 * @return int8_t Error code resulting from reading GPIO expander pins over I2C or mutex acquisition
 */
int8_t read_fuses(pdu_t *pdu, uint8_t fuse_data[2]);

/**
 * @brief Read the state of the TSMS signal.
 * 
 * @param pdu Struct representing the PDU.
 * @param status Pointer to location in memory where value will be read to.
 * @return int8_t Error code.
 */
int8_t read_tsms_sense(pdu_t *pdu, bool *status);

/**
 * @brief Read the status of the shutdown loop.
 * 
 * @param pdu Pointer to struct representing the PDU
 * @return int8_t Result of reading pins on the shutdown monitor GPIO expander of the PDU or result of mutex acquisition
 */
int8_t read_shutdown(pdu_t *pdu, uint8_t shutdown_data[1]);

/**
 * @brief Read the status of the shutdown loop.
 * 
 * @param pdu Pointer to struct representing the PDU
 * @param status Buffer that the data from both sensors will be written to
 */
void read_pump_sensors(pdu_t *pdu, uint16_t pump_sensors_buf[2]);

// Function for reading current
int8_t read_all_current(pdu_t *pdu, float *motor_controller_current,
			float *battbox_fans_current, float *pumps_current,
			float *lv_boards_current);

/**
 * @brief Read the status of brakes
 * 
 * @param pdu Pointer to struct representing the PDU
 * @param status Buffer that fuse data will be written to
 * @return int8_t Error code.
 */
int8_t read_brake_state(pdu_t *pdu, bool *status);

/**
 * @brief writes to config registers of the shutdown and ctrl expanders on pdu
 * 
 * @param pdu Pointer to struct representing the PDU
 * @return error code
 */
uint8_t write_tca_config(pdu_t *pdu);

/**
 * @brief returns whether tca configs have been written too
 * 
 * @param pdu Pointer to struct representing the PDU
 * @return true if correct config read, false otherwise
 */
bool verify_tca_config(pdu_t *pdu);

/**
 * @brief Read the status of all expander debug pins (both ctrl and shutdown, in that order).
 * 
 * @param pdu Pointer to struct representing the PDU
 * @param expander_debug_data Buffer that the data from both expanders will be written to
 * @return int8_t Error code.
 */
int8_t read_expander_debug(pdu_t *pdu, uint8_t expander_debug_data[4]);

/**
 * @brief Taskf for sounding RTDS.
 * 
 * @param arg Pointer to struct representing the PDU.
 */
void vRTDS(void *arg);
extern osThreadId_t rtds_thread;
extern const osThreadAttr_t rtds_attributes;

/* Current Sensors */
#define MOTOR_CONTROLLER_CURRENT_SENSOR_ADDR 0x80
#define BATTBOX_FANS_CURRENT_SENSOR_ADDR     0x82
#define PUMPS_CURRENT_SENSOR_ADDR	     0x88
#define LV_BOARDS_CURRENT_SENSOR_ADDR	     0x8A

/* Misc */
#define MUTEX_TIMEOUT	osWaitForever /* ms */
#define RTDS_DURATION	1750 /* ms at 1kHz tick rate */
#define SOUND_RTDS_FLAG 1U

/* Function that approximates the pump sensor temperature. Takes in resistance and outputs temperature. */
// (Created based on "GE Series RvT" PDU Altium table)
#define PUMP_TEMP_APPROX(R)                                      \
	(-0.1102 * pow(log(R), 3)) + (4.6521 * pow(log(R), 2)) - \
		(80.47 * log(R)) +                               \
		457.6 // f(x) = -0.1102ln(x)^3 + 4.6521ln(x)^2 - 80.47ln(x) + 457.6

/* GPIO Expander Reset Pins */
#define CTRL_RESET_PIN	   GPIO_PIN_6
#define SHUTDOWN_RESET_PIN GPIO_PIN_7

// clang-format off
/* CTRL Expander */
#define CTRL_ADDR		 			TCA_I2C_ADDR_2
#define PIN_PUMP_CTRL_1		 		0 // P00
#define PIN_PUMP_CTRL_2		 		1 // P01
#define PIN_BRKLIGHT_CTRL			2 // P02
#define PIN_FANBATTBOX_CTRL	 		3 // P03
#define PIN_RTDS_CTRL 				4 // P04
#define PIN_RADFAN_CTRL_1			5 // P05
#define PIN_RADFAN_CTRL_2			6 // P06
#define PIN_BATTBOX_FUSE_STAT		7 // P07
#define PIN_LV_BOARDS_FUSE_STAT		0 // P10
#define PIN_RADFAN_FUSE_STAT		1 // P11
#define PIN_FANBATTBOX_FUSE_STAT	2 // P12
#define PIN_DASHBOARD_FUSE_STAT	 	3 // P13
#define PIN_BRKLIGHT_FUSE_STAT	 	4 // P14
#define PIN_SD_TO_BRB_FUSE_STAT	 	5 // P15
#define PIN_PUMP_FUSE_STAT1	 		6 // P16
#define PIN_PUMP_FUSE_STAT2		 	7 // P17

/* Shutdown Expander */
#define SHUTDOWN_ADDR	    TCA_I2C_ADDR_3
#define PIN_CKPT_BRB_CLR    0 // P00
#define PIN_BMS_GOOD	    1 // P01
#define PIN_INERTIA_SW_GOOD 2 // P02
#define PIN_SPARE_GPIO1	    3 // P03
#define PIN_IMD_GOOD	    4 // P04
#define PIN_BSPD_GOOD	    5 // P05
#define PIN_SHUTDOWN_06	    6 // P06 (X)
#define PIN_SHUTDOWN_07	    7 // P07 (X)
#define PIN_SHUTDOWN_10	    0 // P10 (X)
#define PIN_MC_STAT	    	1 // P11
#define PIN_SPARE_IN	    2 // P12
#define PIN_SHUTDOWN_13	    3 // P13
#define PIN_TSMS_SENSE	    4 // P14
#define PIN_BOTS_GOOD	    5 // P15
#define PIN_HVD_INTLK_GOOD  6 // P16
#define PIN_HVC_INTLK_GOOD  7 // P17

#endif /* PDU_H */
