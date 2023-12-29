#ifndef PDU_H
#define PDU_H

#include <stdint.h>
#include <stdbool.h>
#include "pi4ioe.h"
#include "cmsis_os.h"

typedef struct {
    I2C_HandleTypeDef *hi2c;
    osMutexId_t *mutex;
    pi4ioe_t *shutdown_expander;
    //max7341 *ctrl_expander;
} pdu_t;

/* Creates a new PDU interface */
pdu_t *init_pdu(I2C_HandleTypeDef *hi2c);

/* Functions to Control PDU */
int8_t write_pump(pdu_t *pdu, bool status);
int8_t write_fan_radiator(pdu_t *pdu, bool status);
int8_t write_brakelight(pdu_t *pdu, bool status);
int8_t write_fan_battbox(pdu_t *pdu, bool status);

/* Function to Read the Status of Fuses from PDU */
typedef enum {
    FUSE_PUMP,
    FUSE_FAN_RADIATOR,
    FUSE_FAN_BATTBOX,
    FUSE_MC,
    FUSE_LVBOX,
    FUSE_DASHBOARD,
    FUSE_BRAKELIGHT,
    FUSE_BRB,
    MAX_FUSES
} fuse_t;

int8_t read_fuse(pdu_t *pdu, fuse_t fuse, bool *status);

int8_t read_tsms_sense(pdu_t *pdu, bool *status);

/* Functions to Read Status of Various Stages of Shutdown Loop */
typedef enum {
    CKPT_BRB_CLR,   /* Cockpit BRB */
    SIDE_BRB_CLR,   /* Side BRB */
    INTERTIA_SW_OK, /* Inertia Switch */
    BOTS_OK,        /* Brake Over Travel Switch */
    BPSD_OK,        /* Brake System Plausbility Device */
    IMD_OK,         /* Insulation Monitoring Device */
    BMS_OK,         /* Battery Management System (Shepherd) */
    TSMS,           /* Tractive System Main Switch */
    HVD_INTLK_OK,   /* HVD Interlock */
    HVC_INTLK_OK    /* HV C Interlock*/
} shutdown_stage_t;

int8_t read_shutdown(pdu_t *pdu, shutdown_stage_t stage, bool *status);

#endif /* PDU_H */
