#ifndef PDU_H
#define PDU_H

#include "cmsis_os.h"
#include "pca9539.h"
#include "INA226.h"
#include <stdbool.h>
#include <stdint.h>

#define SOUND_RTDS_FLAG 1U

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
int8_t write_pump(pdu_t *pdu, bool status);
int8_t write_brakelight(pdu_t *pdu, bool status);
int8_t write_fan_battbox(pdu_t *pdu, bool status);

/* Function to Read the Status of Fuses from PDU */
typedef enum {
	BATTBOX_FUSE_STAT,
	LV_BOARDS_FUSE_STAT,
	RADFAN_FUSE_STAT,
	BUCK_FUSE_STAT,
	FANBATTBOX_FUSE_STAT,
	PUMP_FUSE_STAT0,
	DASHBOARD_FUSE_STAT,
	BRKLIGHT_FUSE_STAT,
	SD_TO_BRB_FUSE_STAT,
	PUMP_FUSE_STAT1,
	MAX_FUSES
} fuse_t;

/**
 * @brief Read the status of the PDU fuses.
 * 
 * @param pdu Pointer to struct representing the PDU
 * @param status Buffer that fuse data will be written to
 * @return int8_t Error code resulting from reading GPIO expander pins over I2C or mutex acquisition
 */
int8_t read_fuses(pdu_t *pdu, bool status[MAX_FUSES]);

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
	CKPT_BRB_CLR, /* Cockpit BRB */
	BMS_GOOD, /* Battery Management System (Shepherd) */
	INERTIA_SW_GOOD, /* Inertia Switch */
	SPARE_GPIO1,
	IMD_GOOD, /* Insulation Monitoring Device */
	BSPD_GOOD, /* Brake System Plausbility Device */
	BOTS_GOOD, /* Brake Over Travel Switch */
	HVD_INTLK_GOOD, /* HVD Interlock */
	HVC_INTLK_GOOD, /* HV C Interlock*/
	//SIDE_BRB_CLR,	/* Side BRB */
	//TSMS,			/* Tractive System Main Switch */
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

#endif /* PDU_H */
