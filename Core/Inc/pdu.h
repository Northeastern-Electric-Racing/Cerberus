#ifndef PDU_H
#define PDU_H

#include <stdint.h>
#include <stdbool.h>
#include "pi4ioe.h"
#include "cmsis_os.h"

typedef struct {
    pi4ioe_t *gpio_exp;
    osMutexId_t *mutex;
} pdu_t;

/* Creates a new PDU interface */
pdu_t *init_pdu(I2C_HandleTypeDef *hi2c);

/* Functions to Control PDU */
int8_t write_pump(pdu_t *pdu, bool status);
int8_t write_fan_radiator(pdu_t *pdu, bool status);
int8_t write_brakelight(pdu_t *pdu, bool status);
int8_t write_fan_battbox(pdu_t *pdu, bool status);

/* Functions to Read the Status of Fuses from PDU */
int8_t read_fuse_pump(pdu_t *pdu);
int8_t read_fuse_fan_radiator(pdu_t *pdu);
int8_t read_fuse_fan_battbox(pdu_t *pdu);
int8_t read_fuse_mc(pdu_t *pdu);
int8_t read_fuse_lvbox(pdu_t *pdu);
int8_t read_fuse_dashboard(pdu_t *pdu);
int8_t read_fuse_brakelight(pdu_t *pdu);
int8_t read_fuse_brb(pdu_t *pdu);
int8_t read_tsms_sense(pdu_t *pdu);

/* Functions to Read Status of Various Stages of Shutdown Loop */
int8_t read_ckpt_brb_clr(pdu_t *pdu);   /* Cockpit BRB */
int8_t read_side_brb_clr(pdu_t *pdu);   /* Side BRB */
int8_t read_inertia_sw_ok(pdu_t *pdu);  /* Inertia Switch */
int8_t read_bots_ok(pdu_t *pdu);        /* Brake Over Travel Switch */
int8_t read_bspd_ok(pdu_t *pdu);        /* Brake System Plausbility Device */
int8_t read_imd_ok(pdu_t *pdu);         /* Insulation Monitoring Device */
int8_t read_bms_ok(pdu_t *pdu);         /* Battery Management System (Shepherd) */
int8_t read_tsms(pdu_t *pdu);           /* Tractive System Main Switch */
int8_t read_hvd_intlk_ok(pdu_t *pdu);   /* HVD Interlock */
int8_t read_hvc_intlk_ok(pdu_t *pdu);   /* HV C Interlock*/

#endif /* PDU_H */
