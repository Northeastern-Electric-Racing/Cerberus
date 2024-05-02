#ifndef PDU_H
#define PDU_H

#include "cmsis_os.h"
#include "pi4ioe.h"
#include "max7314.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	I2C_HandleTypeDef* hi2c;
	osMutexId_t* mutex;
	max7314_t* shutdown_expander;
	max7314_t* ctrl_expander;
	osTimerId rtds_timer;
} pdu_t;

/* Creates a new PDU interface */
pdu_t* init_pdu(I2C_HandleTypeDef* hi2c);

/* Functions to Control PDU */
int8_t write_pump(pdu_t* pdu, bool status);
int8_t write_fault(pdu_t* pdu, bool status);
int8_t write_brakelight(pdu_t* pdu, bool status);
int8_t write_fan_battbox(pdu_t* pdu, bool status);
int8_t sound_rtds(pdu_t* pdu);
int8_t write_rtds(pdu_t* pdu, bool status);

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

int8_t read_fuse(pdu_t* pdu, fuse_t fuse, bool* status);

int8_t read_tsms_sense(pdu_t* pdu, bool* status);

/* Functions to Read Status of Various Stages of Shutdown Loop */
typedef enum {
	CKPT_BRB_CLR,	/* Cockpit BRB */
    BMS_OK,			/* Battery Management System (Shepherd) */
    INERTIA_SW_OK, /* Inertia Switch */
    SPARE_GPIO1_OK,
    IMD_OK,			/* Insulation Monitoring Device */
    BSPD_OK,		/* Brake System Plausbility Device */
	BOTS_OK,		/* Brake Over Travel Switch */
	HVD_INTLK_OK,	/* HVD Interlock */
	HVC_INTLK_OK,	/* HV C Interlock*/
	MAX_SHUTDOWN_STAGES
} shutdown_stage_t;

int8_t read_shutdown(pdu_t* pdu, shutdown_stage_t stage, bool* status);

#endif /* PDU_H */
