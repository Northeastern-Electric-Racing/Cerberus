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

/* Function to read the status of Tractive System Main Switch */
int8_t read_tsms_sense(pdu_t *pdu);

#endif /* PDU_H */
