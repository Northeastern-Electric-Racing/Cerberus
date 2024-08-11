#ifndef PDU_H
#define PDU_H

#include "cmsis_os.h"
#include "pca9539.h"
#include <stdbool.h>
#include <stdint.h>

#define SOUND_RTDS_FLAG 1U

typedef struct {
	I2C_HandleTypeDef *hi2c;
	osMutexId_t *mutex;
	pca9539_t *shutdown_expander;
	pca9539_t *ctrl_expander;
} pdu_t;

/* Creates a new PDU interface */
pdu_t *init_pdu(I2C_HandleTypeDef *hi2c);

/* Functions to Control PDU */
int8_t write_pump(pdu_t *pdu, bool status);
int8_t write_fault(pdu_t *pdu, bool status);
int8_t write_brakelight(pdu_t *pdu, bool status);
int8_t write_fan_battbox(pdu_t *pdu, bool status);

/* Function to Read the Status of Fuses from PDU */
typedef enum {
	FUSE_BATTBOX,
	FUSE_LVBOX,
	FUSE_FAN_RADIATOR,
	FUSE_MC,
	FUSE_FAN_BATTBOX,
	FUSE_PUMP,
	FUSE_DASHBOARD,
	FUSE_BRAKELIGHT,
	FUSE_BRB,
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
	AMS_OK, /* Battery Management System (Shepherd) */
	INERTIA_SW_OK, /* Inertia Switch */
	SPARE_GPIO1_OK,
	IMD_OK, /* Insulation Monitoring Device */
	BSPD_OK, /* Brake System Plausbility Device */
	BOTS_OK, /* Brake Over Travel Switch */
	HVD_INTLK_OK, /* HVD Interlock */
	HVC_INTLK_OK, /* HV C Interlock*/
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
 * @brief Taskf for sounding RTDS.
 * 
 * @param arg Pointer to struct representing the PDU.
 */
void vRTDS(void *arg);
extern osThreadId_t rtds_thread;
extern const osThreadAttr_t rtds_attributes;

#endif /* PDU_H */
