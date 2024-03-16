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
} pdu_t;

/* Creates a new PDU interface */
pdu_t* init_pdu(I2C_HandleTypeDef* hi2c);

/* Functions to Control PDU */
int8_t write_pump(pdu_t* pdu, bool status);
int8_t write_fan_radiator(pdu_t* pdu, bool status);
int8_t write_brakelight(pdu_t* pdu, bool status);
int8_t write_fan_battbox(pdu_t* pdu, bool status);

/* Function to Read the Status of Fuses from PDU */
typedef enum {
    FUSE_BATTBOX = 4,
    FUSE_LVBOX = 5,
    FUSE_FAN_RADIATOR = 6,
    FUSE_MC = 7,
    FUSE_FAN_BATTBOX =  8,
	FUSE_PUMP = 9,
	FUSE_DASHBOARD = 10,
	FUSE_BRAKELIGHT = 11,
	FUSE_BRB = 12,
	MAX_FUSES = 9
} fuse_t;

int8_t read_fuse(pdu_t* pdu, fuse_t fuse, bool* status);

int8_t read_tsms_sense(pdu_t* pdu, bool* status);

/* Functions to Read Status of Various Stages of Shutdown Loop */
typedef enum {
	CKPT_BRB_CLR = 0,	/* Cockpit BRB */
    BMS_OK = 2,			/* Battery Management System (Shepherd) */
    INTERTIA_SW_OK = 3, /* Inertia Switch */
    SPARE_GPIO1_OK = 4,
    IMD_OK = 5,			/* Insulation Monitoring Device */
    BPSD_OK = 8,		/* Brake System Plausbility Device */
	BOTS_OK = 13,		/* Brake Over Travel Switch */
	HVD_INTLK_OK = 14,	/* HVD Interlock */
	HVC_INTLK_OK = 15,	/* HV C Interlock*/
    SIDE_BRB_CLR,	/* Side BRB */
    TSMS,			/* Tractive System Main Switch */
	MAX_SHUTDOWN_STAGES = 9
} shutdown_stage_t;

int8_t read_shutdown(pdu_t* pdu, shutdown_stage_t stage, bool* status);

#endif /* PDU_H */
