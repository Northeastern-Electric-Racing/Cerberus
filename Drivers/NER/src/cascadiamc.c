/**
 * @file cascadiamc.c
 * @author Hamza Iqbal
 * @brief Source file for Motor Controller Driver
 * @version 0.1
 * @date 2023-08-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cascadiamc.h"

void cascadiamc_init(cascadiamc_t* mc)
{

}

void cascadiamc_update(cascadiamc_t* mc, can_msg_t message)
{
    switch(message.id)
    {
        case 0x2010:    // The byte furthest to the right is a standin for now, should be the last 2 digits of serial number in hex
            int32_t ERPM = message.data >> 32;
            mc->rpm = ERPM / NUM_POLES;
            mc->duty_cycle = ((message.data >> 16) & 0x00000000FFFF);
            mc->input_voltage = message.data & 0x00000000FFFF;
        case 0x2110:    // The byte furthest to the right is a standin for now, should be the last 2 digits of serial number in hex
            mc->ac_current = message.data >> 48;
            mc->dc_current = ((message.data >> 32) & 0x00000000FFFF);
        case 0x2210:    // The byte furthest to the right is a standin for now, should be the last 2 digits of serial number in hex
            mc->cont_temp = message.data >> 48;
            mc->motor_temp = ((message.data >> 32) & 0x00000000FFFF);
            mc->fault_code = ((message.data >> 24) & 0x0000000000FF);
        case 0x2310:    // The byte furthest to the right is a standin for now, should be the last 2 digits of serial number in hex
            mc->throttle_signal = message.data >> 56;
            mc->brake_signal = ((message.data >> 48) & 0x0000000000FF);
            mc->drive_enable = ((message.data >> 39) & 0x000000000001);
    }
}

int32_t cascadiamc_get_rpm(cascadiamc_t* mc)
{
    return mc->rpm;
}

int16_t cascadiamc_get_duty_cycle(cascadiamc_t* mc)
{
    return mc->duty_cycle;
}

int16_t cascadiamc_get_input_voltage(cascadiamc_t* mc)
{
    return mc->input_voltage;
}

int16_t cascadiamc_get_ac_current(cascadiamc_t* mc)
{
    return mc->ac_current;
}

int16_t cascadiamc_get_dc_current(cascadiamc_t* mc)
{
    return mc->dc_current;
}

int16_t cascadiamc_get_controller_temp(cascadiamc_t* mc)
{
    return mc->cont_temp;
}

int16_t cascadiamc_get_motor_temp(cascadiamc_t* mc)
{
    return mc->motor_temp;
}

uint8_t cascadiamc_get_fault_code(cascadiamc_t* mc)
{
    return mc->fault_code;
}

int8_t cascadiamc_get_throttle_signal(cascadiamc_t* mc)
{
    return mc->throttle_signal;
}

int8_t cascadiamc_get_brake_signal(cascadiamc_t* mc)
{
    return mc->brake_signal;
}

int8_t cascadiamc_get_drive_enable(cascadiamc_t* mc)
{
    return mc->drive_enable;
}

void cascadiamc_set_current(int16_t current)                                /* SCALE: 10          UNITS: Amps       */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint16_t ac_current;
        } cfg;

    } set_current_data;

    uint32_t msg_id = 0x00000110;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_current_data.cfg.ac_current = current;
    can_msg_t current_msg;
    current_msg.data = set_current_data.cfg;
    current_msg.line = msg_line;
    current_msg.len = msg_len;
    current_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(current_msg);
}

void cascadiamc_set_brake_current(int16_t brake_current)                    /* SCALE: 10          UNITS: Amps       */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint16_t brake_current;
        } cfg;

    } set_brake_current_data;

    uint32_t msg_id = 0x00000210;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_brake_current_data.cfg.brake_current = brake_current;
    can_msg_t brake_current_msg;
    brake_current_msg.data = set_brake_current_data.cfg;
    brake_current_msg.line = msg_line;
    brake_current_msg.len = msg_len;
    brake_current_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(brake_current_msg);
}

void cascadiamc_set_speed(int32_t rpm)                                      /* SCALE: 1           UNITS: RPM        */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint32_t erpm;
        } cfg;

    } set_speed_data;

    uint32_t msg_id = 0x00000310;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_speed_data.cfg.erpm = rpm * NUM_POLES;
    can_msg_t speed_msg;
    speed_msg.data = set_speed_data.cfg;
    speed_msg.line = msg_line;
    current_msg.len = msg_len;
    current_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(speed_msg);
}

void cascadiamc_set_position(int16_t angle)                                 /* SCALE: 10          UNITS: Degrees    */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint16_t angle;
        } cfg;

    } set_position_data;

    uint32_t msg_id = 0x00000410;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_position_data.cfg.angle = angle;
    can_msg_t position_msg;
    position_msg.data = set_position_data.cfg;
    position_msg.line = msg_line;
    position_msg.len = msg_len;
    position_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(current_msg);
}

void cascadiamc_set_relative_current(int16_t relative_current)              /* SCALE: 10          UNITS: Percentage */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint16_t relative_current;
        } cfg;

    } set_relative_current_data;

    uint32_t msg_id = 0x00000510;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_relative_current_data.cfg.relative_current = relative_current;
    can_msg_t relative_current_msg;
    relative_current_msg.data = set_relative_current_data.cfg;
    relative_current_msg.line = msg_line;
    relative_current_msg.len = msg_len;
    relative_current_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(relative_current_msg);
}

void cascadiamc_set_relative_brake_current(int16_t relative_brake_current)  /* SCALE: 10          UNITS: Percentage */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint16_t relative_brake_current;
        } cfg;

    } set_relative_brake_current_data;

    uint32_t msg_id = 0x00000610;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_relative_brake_current_data.cfg.relative_brake_current = relative_brake_current;
    can_msg_t relative_brake_current_msg;
    relative_brake_current_msg.data = set_relative_brake_current_data.cfg;
    relative_brake_current_msg.line = msg_line;
    relative_brake_current_msg.len = msg_len;
    relative_brake_current_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(relative_brake_current_msg);
}

void cascadiamc_set_digital_output(uint8_t output, bool value)              /* SCALE: 1           UNITS: No units   */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint8_t digital_output;
        } cfg;

    } set_digital_output_data;

    uint32_t msg_id = 0x00000710;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_digital_output_data.cfg.digital_output = value >> output;
    can_msg_t digital_output_msg;
    digital_output_msg.data = set_digital_output_data.cfg;
    digital_output_msg.line = msg_line;
    digital_output_msg.len = msg_len;
    digital_output_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(digital_output_msg);
}

void cascadiamc_set_max_ac_current(int16_t current)                         /* SCALE: 10          UNITS: Amps       */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint16_t max_current;
        } cfg;

    } set_max_ac_current_data;

    uint32_t msg_id = 0x00000810;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_max_ac_current_data.cfg.max_current = current;
    can_msg_t max_ac_current_msg;
    max_ac_current_msg.data = set_max_ac_current_data.cfg;
    max_ac_current_msg.line = msg_line;
    max_ac_current_msg.len = msg_len;
    max_ac_current_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(max_ac_current_msg);
}

void cascadiamc_set_max_ac_brake_current(int16_t current)                   /* SCALE: 10          UNITS: Amps       */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint16_t max_brake_current;
        } cfg;

    } set_max_brake_ac_current_data;

    uint32_t msg_id = 0x00000910;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_max_ac_brake_current_data.cfg.max_brake_current = current;
    can_msg_t max_ac_brake_current_msg;
    max_ac_brake_current_msg.data = set_max_ac_current_data.cfg;
    max_ac_brake_current_msg.line = msg_line;
    max_ac_brake_current_msg.len = msg_len;
    max_ac_brake_current_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(max_ac_brake_current_msg);
}

void cascadiamc_set_max_dc_current(int16_t current)                         /* SCALE: 10          UNITS: Amps       */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint16_t max_current;
        } cfg;

    } set_max_dc_current_data;

    uint32_t msg_id = 0x00000A10;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_max_dc_current_data.cfg.max_current = current;
    can_msg_t max_dc_current_msg;
    max_dc_current_msg.data = set_max_dc_current_data.cfg;
    max_dc_current_msg.line = msg_line;
    max_dc_current_msg.len = msg_len;
    max_dc_current_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(max_dc_current_msg);
}

void cascadiamc_set_max_dc_brake_current(int16_t current)                   /* SCALE: 10          UNITS: Amps       */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint16_t max_brake_current;
        } cfg;

    } set_max_brake_ac_current_data;

    uint32_t msg_id = 0x00000B10;
    uint8_t msg_len = 8;
    uint8_t msg_line = 1;
    set_max_ac_brake_current_data.cfg.max_brake_current = current;
    can_msg_t max_ac_brake_current_msg;
    max_ac_brake_current_msg.data = set_max_ac_current_data.cfg;
    max_ac_brake_current_msg.line = msg_line;
    max_ac_brake_current_msg.len = msg_len;
    max_ac_brake_current_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(max_ac_brake_current_msg);
}

void cascadiamc_set_drive_enable(bool drive_enable)                         /* SCALE: 1           UNITS: No units   */
{
    union
    {
        uint8_t msg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        struct
        {
            uint8_t drive_enable;
        } cfg;

    } set_drive_enable_data;

    uint32_t msg_id = 0x00000C10;
    uint8_t msg_len = 1;
    uint8_t msg_line = 1;
    set_drive_enable_data.cfg.drive_enable = drive_enable;
    can_msg_t drive_enable_msg;
    drive_enable_msg.data = set_max_ac_current_data.cfg;
    drive_enable_msg.line = msg_line;
    drive_enable_msg.len = msg_len;
    drive_enable_msg.id = msg_id;

    CAN_StatusTypedef status = can_send_message(drive_enable_msg);
}