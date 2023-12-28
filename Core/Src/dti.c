/**
 * @file dti.c
 * @author Hamza Iqbal
 * @brief Source file for Motor Controller Driver
 * @version 0.1
 * @date 2023-08-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <string.h>
#include "dti.h"
#include "can.h"

void dti_init(dti_t* mc)
{

}

void dti_set_current(int16_t current)
{
    can_msg_t msg
		= { .id = 0x00000110, .len = 2, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &current, msg.len);
    queue_can_msg(msg);
}

void dti_set_brake_current(int16_t brake_current)
{
    can_msg_t msg
		= { .id = 0x00000210, .len = 2, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &brake_current, msg.len);
    queue_can_msg(msg);
}

void dti_set_speed(int32_t rpm)
{
    can_msg_t msg
		= { .id = 0x00000310, .len = 4, .data = { 0 } };

    rpm = rpm * NUM_POLES;

    /* Send CAN message */
    memcpy(msg.data, &rpm, msg.len);
    queue_can_msg(msg);
}

void dti_set_position(int16_t angle)
{
    can_msg_t msg
		= { .id = 0x00000410, .len = 2, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &angle, msg.len);
    queue_can_msg(msg);
}

void dti_set_relative_current(int16_t relative_current)
{
    can_msg_t msg
		= { .id = 0x00000510, .len = 2, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &relative_current, msg.len);
    queue_can_msg(msg);
}

void dti_set_relative_brake_current(int16_t relative_brake_current)
{
    can_msg_t msg
		= { .id = 0x00000610, .len = 2, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &relative_brake_current, msg.len);
    queue_can_msg(msg);
}

void dti_set_digital_output(uint8_t output, bool value)
{
    can_msg_t msg
		= { .id = 0x00000710, .len = 1, .data = { 0 } };

    uint8_t ctrl = value >> output;

    /* Send CAN message */
    memcpy(msg.data, &ctrl, msg.len);
    queue_can_msg(msg);
}

void dti_set_max_ac_current(int16_t current)
{
    can_msg_t msg
		= { .id = 0x00000810, .len = 2, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &current, msg.len);
    queue_can_msg(msg);
}

void dti_set_max_ac_brake_current(int16_t current)
{
    can_msg_t msg
		= { .id = 0x00000910, .len = 2, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &current, msg.len);
    queue_can_msg(msg);
}

void dti_set_max_dc_current(int16_t current)
{
    can_msg_t msg
		= { .id = 0x00000A10, .len = 2, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &current, msg.len);
    queue_can_msg(msg);
}

void dti_set_max_dc_brake_current(int16_t current)
{
    can_msg_t msg
		= { .id = 0x00000B10, .len = 2, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &current, msg.len);
    queue_can_msg(msg);
}

void dti_set_drive_enable(bool drive_enable)
{
    can_msg_t msg
		= { .id = 0x00000C10, .len = 1, .data = { 0 } };

    /* Send CAN message */
    memcpy(msg.data, &drive_enable, msg.len);
    queue_can_msg(msg);
}