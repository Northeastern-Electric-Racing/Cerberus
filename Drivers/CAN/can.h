/**
 * @file can.h
 * @author Hamza Iqbal (iqbal.ha@northeastern.edu)
 * @brief CAN Driver
 * @date 2023-06-14
 * 
 * https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf
 * CAN Documentation can be found in pages 87-113
 */

#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include "stm32f4xx_hal.h"
#include "can_config.h"



/*
    Type Definitions
    *************************************************************
*/
typedef struct
{
    CAN_HandleTypeDef* CAN_handler;
    uint32_t id = 0;          // can identifier
    uint8_t len = 8;      `   // length of data
    uint8_t data[8] = { 0 };  // data
    uint8_t bus = 0;          // used to identify where the message came from when events() is used.
    
} CAN_msg_t;

/*
    Global Variables and enums
    *************************************************************
*/

CAN_HandleTypeDef* CAN1;
CAN_HandleTypeDef* CAN2;
CAN_HandleTypeDef* CAN3;




enum
{
    CAN_line_1 = 1,
    CAN_line_2 = 2,
    CAN_line_3 = 3
};

/*
    Function Definitions
    *************************************************************
*/

// Initializes the can handlers for each line
void start_CAN();

int send_message_CAN_1(uint32_t id, uint8_t len, const uint8_t *data);

int send_message_CAN_2(uint32_t id, uint8_t len, const uint8_t *data);

int send_message_CAN_3(uint32_t id, uint8_t len, const uint8_t *data);

void incoming_callback_CAN(const CAN_msg_t &msg);


/*
    CAN Message Handlers
*/

void CAN_BMS_acc_status             (const CAN_msg_t &msg);
void CAN_BMS_cell_data              (const CAN_msg_t &msg);
void CAN_BMS_current_limits         (const CAN_msg_t &msg);
void CAN_BMS_shutdown               (const CAN_msg_t &msg);
void CAN_BMS_DTC_status             (const CAN_msg_t &msg);
void CAN_acceleration_ctrl          (const CAN_msg_t &msg);
void CAN_motor_temp_1               (const CAN_msg_t &msg);
void CAN_motor_temp_2               (const CAN_msg_t &msg);
void CAN_motor_temp_3               (const CAN_msg_t &msg);
void CAN_motor_motion               (const CAN_msg_t &msg);
void CAN_motor_current              (const CAN_msg_t &msg);
void CAN_motor_voltage              (const CAN_msg_t &msg);
void CAN_MC_vehicle_state           (const CAN_msg_t &msg);
void CAN_MC_fault                   (const CAN_msg_t &msg);
void CAN_motor_torque_timer         (const CAN_msg_t &msg);
void CAN_BMS_status_2               (const CAN_msg_t &msg);
void CAN_BMS_charge_discharge       (const CAN_msg_t &msg);
void CAN_MC_BMS_integration         (const CAN_msg_t &msg);
void CAN_MC_set_parameter           (const CAN_msg_t &msg);
void CAN_BMS_charging_state         (const CAN_msg_t &msg);
void CAN_BMS_currents               (const CAN_msg_t &msg);

#endif
