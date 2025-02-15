#ifndef PDU_H
#define PDU_H

#include <stdbool.h>
#include "fault.h"
#include "cmsis_os.h"
#include "pca9539.h"
#include "INA226.h"
#include <stdbool.h>
#include <stdint.h>
#include "bitstream.h"
#include "c_utils.h"

typedef struct {
	I2C_HandleTypeDef *hi2c;
	osMutexId_t *mutex;
	pca9539_t *shutdown_expander;
	pca9539_t *ctrl_expander;

	ina226_t *motor_controller_current_sensor;
	ina226_t *battbox_fans_current_sensor;
	ina226_t *pumps_current_sensor;
	ina226_t *lv_boards_current_sensor;

	ADC_HandleTypeDef *pump_sensors_adc;
	uint32_t pump_sensors_dma_buf[2];
} pdu_t;

/* Creates a new PDU interface */
pdu_t *init_pdu(I2C_HandleTypeDef *hi2c, ADC_HandleTypeDef *pump_sensors_adc);

/* Functions to Control PDU */
int8_t write_pump_0(pdu_t *pdu, bool state);
int8_t write_pump_1(pdu_t *pdu, bool state);
int8_t write_24V_12V_buck(pdu_t *pdu, bool state);
int8_t write_brakelight(pdu_t *pdu, bool state);
int8_t write_fan_battbox(pdu_t *pdu, bool state);
int8_t write_rtds(pdu_t *pdu, bool state);
int8_t write_radfan_0(pdu_t *pdu, bool state);
int8_t write_radfan_1(pdu_t *pdu, bool state);

/* Function to Read the Status of Fuses from PDU */
typedef enum {
	PUMP_FUSE_STAT0,
	SD_TO_BRB_FUSE,
	LV_BOARDS_FUSE_STAT,
	RADFAN_FUSE_STAT,
	BATTBOX_FUSE_STAT,
	BUCK_FUSE_STAT,
	FANBATTBOX_STAT,
	PUMP_FUSE_STAT1,
	DASHBOARD_FUSE_STAT,
	BRKLIGHT_FUSE_STAT,
	MAX_FUSES
} fuse_t;

/**
 * @brief Read the status of the PDU fuses.
 * 
 * @param pdu Pointer to struct representing the PDU
 * @param status Bitstream for storing fuse data
 * @return int8_t Error code resulting from reading GPIO expander pins over I2C or mutex acquisition
 */
int8_t read_fuses(pdu_t *pdu, bitstream_t* bitstream);

/**
 * @brief Read the state of the TSMS signal.
 * 
 * @param pdu Struct representing the PDU.
 * @param status Pointer to location in memory where value will be read to.
 * @return int8_t Error code.
 */
int8_t read_tsms_sense(pdu_t *pdu, bool *status);

/* Functions to Read Status of Various Stages of Shutdown Loop */
typedef enum {
	HVD_GOOD,
	HVC_GOOD,
	BOTS_GOOD,
	CKPT_BRB,
	BMS_GOOD,
	INERTIA_SW_GOOD,
	SPARE_GPIO0,
	IMD_GOOD,
	BSPD_GOOD,
	MAX_SHUTDOWN_STAGES
} shutdown_stage_t;

/**
 * @brief Read the status of the shutdown loop.
 * 
 * @param pdu Pointer to struct representing the PDU
 * @param status Buffer that fuse data will be written to
 * @return int8_t Result of reading pins on the shutdown monitor GPIO expander of the PDU or result of mutex acquisition
 */
int8_t read_shutdown(pdu_t *pdu, bool status[MAX_SHUTDOWN_STAGES]);

/**
 * @brief Read the status of the shutdown loop.
 * 
 * @param pdu Pointer to struct representing the PDU
 * @param status Buffer that the data from both sensors will be written to
 */
void read_pump_sensors(pdu_t *pdu, uint32_t pump_sensors_buf[2]);

// Function for reading current
int8_t read_all_current(pdu_t *pdu, float *motor_controller_current,
			float *battbox_fans_current, float *pumps_current,
			float *lv_boards_current);

/**
 * @brief Taskf for sounding RTDS.
 * 
 * @param arg Pointer to struct representing the PDU.
 */
void vRTDS(void *arg);
extern osThreadId_t rtds_thread;
extern const osThreadAttr_t rtds_attributes;

/**
 * @brief Read the status of brakes
 * 
 * @param pdu Pointer to struct representing the PDU
 * @param status Buffer that fuse data will be written to
 * @return int8_t Error code.
 */
int8_t read_brake_state(pdu_t *pdu, bool *status);

/* Current Sensors */
#define MOTOR_CONTROLLER_CURRENT_SENSOR_ADDR 0x40
#define BATTBOX_FANS_CURRENT_SENSOR_ADDR     0x42
#define PUMPS_CURRENT_SENSOR_ADDR	     0x44
#define LV_BOARDS_CURRENT_SENSOR_ADDR	     0x45

/* Misc */
#define MUTEX_TIMEOUT	osWaitForever /* ms */
#define RTDS_DURATION	1750 /* ms at 1kHz tick rate */
#define SOUND_RTDS_FLAG 1U

// Temp stuff
/* CTRL Expander */
#define CTRL_ADDR		PCA_I2C_ADDR_0
#define PIN_PUMP_FUSE_STAT0	0 // P00
#define PIN_RTD_CTRL		1 // P01
#define PIN_SD_TO_BRB_FUSE_STAT 2 // P02
#define PIN_PUMP_CTRL0		3 // P03
#define PIN_PUMP_CTRL1		4 // P04
#define PIN_BUCK_CTRL		5 // P05
#define PIN_BRKLIGHT_CTRL	6 // P06
#define PIN_FANBATTBOX_CTRL	7 // P07
#define PIN_LV_BOARDS_FUSE_STAT 0 // P10
#define PIN_RADFAN_FUSE_STAT	1 // P11
#define PIN_BATTBOX_FUSE_STAT	2 // P12
#define PIN_BUCK_FUSE_STAT	3 // P13
#define PIN_FANBATTBOX_STAT	4 // P14
#define PIN_PUMP_FUSE_STAT1	5 // P15
#define PIN_DASHBOARD_FUSE_STAT 6 // P16
#define PIN_BRKLIGHT_FUSE_STAT	7 // P17

/* Shutdown Expander */
#define SHUTDOWN_ADDR	    PCA_I2C_ADDR_1
#define PIN_HVD_GOOD	    0 // P00
#define PIN_HVC_GOOD	    1 // P01
#define PIN_BOTS_GOOD	    2 // P02
#define PIN_CKPT_BRB	    3 // P03
#define PIN_BMS_GOOD	    4 // P04
#define PIN_INERTIA_SW_GOOD 5 // P05
#define PIN_SPARE_GPIO0	    6 // P06
#define PIN_IMD_GOOD	    7 // P07
#define PIN_SHUTDOWN_10	    0 // P10 (x)
#define PIN_SHUTDOWN_11	    1 // P11 (x)
#define PIN_BSPD_GOOD	    2 // P12
#define PIN_SHUTDOWN_13	    3 // P13 (x)
#define PIN_MC_STAT	    4 // P14
#define PIN_SPARE_1	    5 // P15
#define PIN_SPARE_2	    6 // P16
#define PIN_TMS_SENSE	    7 // P17

// Stuff that will be used eventually
/* 
CTRL Expander
#define CTRL_ADDR		 PCA_I2C_ADDR_0
#define PIN_PUMP_CTRL_0		 0 // P00
#define PIN_PUMP_CTRL_1		 1 // P01
#define PIN_24V_12V_BUCK_CTRL	 2 // P02
#define PIN_BRKLIGHT_CTRL	 3 // P03
#define PIN_FANBATTBOX_CTRL	 4 // P04
#define PIN_BATTBOX_FUSE_STAT	 5 // P05
#define PIN_LV_BOARDS_FUSE_STAT	 6 // P06
#define PIN_RADFAN_FUSE_STAT	 7 // P07
#define PIN_BUCK_FUSE_STAT	 0 // P10
#define PIN_FANBATTBOX_FUSE_STAT 1 // P11
#define PIN_PUMP_FUSE_STAT0	 2 // P12
#define PIN_DASHBOARD_FUSE_STAT	 3 // P13
#define PIN_BRKLIGHT_FUSE_STAT	 4 // P14
#define PIN_SD_TO_BRB_FUSE_STAT	 5 // P15
#define PIN_PUMP_FUSE_STAT1	 6 // P16
#define PIN_RTDS_CTRL		 7 // P17

Shutdown Expander
#define SHUTDOWN_ADDR	    PCA_I2C_ADDR_1
#define PIN_CKPT_BRB_CLR    0 // P00
#define PIN_BMS_GOOD	    1 // P01
#define PIN_INERTIA_SW_GOOD 2 // P02
#define PIN_SPARE_GPIO1	    3 // P03
#define PIN_IMD_GOOD	    4 // P04
#define PIN_BSPD_GOOD	    5 // P05
#define PIN_SHUTDOWN_06	    6 // P06 (X)
#define PIN_SHUTDOWN_07	    7 // P07 (X)
#define PIN_SHUTDOWN_10	    0 // P10 (X)
#define PIN_MC_STAT	    1 // P11
#define PIN_SPARE_MON	    2 // P12
#define PIN_SPARE_FAULT	    3 // P13
#define PIN_TSMS_SENSE	    4 // P14
#define PIN_BOTS_GOOD	    5 // P15
#define PIN_HVD_INTLK_GOOD  6 // P16
#define PIN_HVC_INTLK_GOOD  7 // P17
*/

#endif /* PDU_H */
