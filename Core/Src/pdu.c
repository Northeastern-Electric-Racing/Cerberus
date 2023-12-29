#include "pdu.h"
#include <assert.h>
#include <stdlib.h>

#define PUMP_CTRL 			0x00
#define RADFAN_CTRL 		0x01
#define BRKLIGHT_CTRL 		0x02
#define BATBOXFAN_CTRL 		0x03
#define TSMS_CTRL 			0x04
#define SMBALERT	 		0x05
#define MUTEX_TIMEOUT       10  /* ms */

static osMutexAttr_t pdu_mutex_attributes;

pdu_t *init_pdu(I2C_HandleTypeDef *hi2c)
{
    assert(hi2c);

    /* Create PDU struct */
    pdu_t *pdu = malloc(sizeof(pdu_t));
    assert(pdu);

    pdu->hi2c = hi2c;

    /* Initialize Shutdown GPIO Expander */
    pdu->shutdown_expander = malloc(sizeof(pi4ioe_t));
    assert(pdu->shutdown_expander);
    if (pi4ioe_init(pdu->shutdown_expander, pdu->hi2c)) {
        free(pdu->shutdown_expander);
        free(pdu);
        return NULL;
    }
    
    /* Initialize Control GPIO Expander */
    //pdu->ctrl_expander = malloc(sizeof(max7431_t));
    //assert(pdu->ctrl_expander);
    //if (max7341_init(pdu->ctrl_expander, pdu->hi2c)) {
    //  free(pdu->ctrl_expander);
    //  free(pdu->shutdown_expander);
    //  free(pdu);
    //  return NULL;
    //}

    /* Create Mutex */
    pdu->mutex = osMutexNew(&pdu_mutex_attributes);
    assert(pdu->mutex);

    return pdu;
}

int8_t write_pump(pdu_t *pdu, bool status)
{
    if (!pdu)
        return -1;

    osStatus_t stat = osMutexAcquire(pdu->mutex, 10);
    if (stat)
        return stat;

    //write pump over i2c

    osMutexRelease(pdu->mutex);
    return 0;
}

int8_t write_fan_radiator(pdu_t *pdu, bool status)
{
    if (!pdu)
        return -1;

    osStatus_t stat = osMutexAcquire(pdu->mutex, 10);
    if (stat)
        return stat;
        
    //write radiator over i2c

    osMutexRelease(pdu->mutex);
    return 0;
}

int8_t write_brakelight(pdu_t *pdu, bool status)
{
    if (!pdu)
        return -1;

    osStatus_t stat = osMutexAcquire(pdu->mutex, 10);
    if (stat)
        return stat;
        
    //write brakelight over i2c

    osMutexRelease(pdu->mutex);
    return 0;
}

int8_t write_fan_battbox(pdu_t *pdu, bool status)
{
    if (!pdu)
        return -1;

    osStatus_t stat = osMutexAcquire(pdu->mutex, 10);
    if (stat)
        return stat;
        
    //write fan over i2c

    osMutexRelease(pdu->mutex);
    return 0;
}

int8_t read_fuse(pdu_t *pdu, fuse_t fuse, bool *status)
{
    if (!pdu)
        return -1;

    osStatus_t stat = osMutexAcquire(pdu->mutex, 10);
    if (stat)
        return stat;
    
    //figure out which fuse to use
    //write fan over i2c

    osMutexRelease(pdu->mutex);
    return 0;
}

int8_t read_tsms_sense(pdu_t *pdu, bool *status)
{
    if (!pdu)
        return -1;

    osStatus_t stat = osMutexAcquire(pdu->mutex, 10);
    if (stat)
        return stat;
    
    //read pin over i2c

    osMutexRelease(pdu->mutex);
    return 0;
}

int8_t read_shutdown(pdu_t *pdu, shutdown_stage_t stage, bool *status)
{
    if (!pdu)
        return -1;

    osStatus_t stat = osMutexAcquire(pdu->mutex, 10);
    if (stat)
        return stat;
    
    //figure out which pin to read
    //read pin over i2c

    osMutexRelease(pdu->mutex);
    return 0;
}
