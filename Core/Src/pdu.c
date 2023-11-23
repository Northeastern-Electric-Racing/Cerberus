#include "pdu.h"

#define PUMP_CTRL 			0x00
#define RADFAN_CTRL 		0x01
#define BRKLIGHT_CTRL 		0x02
#define BATBOXFAN_CTRL 		0x03
#define TSMS_CTRL 			0x04
#define SMBALERT	 		0x05

pdu_t *init_pdu(I2C_HandleTypeDef *hi2c);

int8_t write_pump(pdu_t *pdu, bool status);
int8_t write_fan_radiator(pdu_t *pdu_t, bool status);
int8_t write_brakelight(pdu_t *pdu, bool status);
int8_t write_fan_battbox(pdu_t *pdu, bool status);
int8_t read_fuse_pump(pdu_t *pdu);
int8_t read_fuse_fan_radiator(pdu_t *pdu);
int8_t read_fuse_fan_battbox(pdu_t *pdu);
int8_t read_fuse_mc(pdu_t *pdu);
int8_t read_fuse_lvbox(pdu_t *pdu);
int8_t read_fuse_dashboard(pdu_t *pdu);
int8_t read_fuse_brakelight(pdu_t *pdu);
int8_t read_fuse_brb(pdu_t *pdu);
int8_t read_tsms_sense(pdu_t *pdu);
