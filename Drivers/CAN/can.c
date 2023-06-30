/*
    CAN DRIVER Source File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf
    
    Author: Hamza Iqbal
*/

#include "can.h"


void CAN_Handler_Init(CAN_HandleTypeDef* CAN_Line)
{
    return;
}


void start_CAN()
{
    return;
}

CAN_msg_t serializeCANMsg(uint32_t id, uint8_t len, uint8_t* data, uint8_t bus, CAN_HandleTypeDef can_line)
{
    return;
}

int sendMessageCAN1(uint32_t id, uint8_t len, const uint8_t *data)
{
    return;
}

int sendMessageCAN2(uint32_t id, uint8_t len, const uint8_t *data)
{
    return;
}

int sendMessageCAN3(uint32_t id, uint8_t len, const uint8_t *data)
{
    return;
}

void incoming_callback_CAN(const CAN_msg_t &msg)
{
    return;
}




__attribute__((weak)) void CAN_BMS_acc_status             (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_BMS_cell_data              (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_BMS_current_limits         (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_BMS_shutdown               (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_BMS_DTC_status             (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_acceleration_ctrl          (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_motor_temp_1               (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_motor_temp_2               (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_motor_temp_3               (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_motor_motion               (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_motor_current              (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_motor_voltage              (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_MC_vehicle_state           (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_MC_fault                   (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_motor_torque_timer         (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_BMS_status_2               (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_BMS_charge_discharge       (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_MC_BMS_integration         (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_MC_set_parameter           (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_BMS_charging_state         (const CAN_msg_t &msg){return;}
__attribute__((weak)) void CAN_BMS_currents               (const CAN_msg_t &msg){return;}
