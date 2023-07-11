/*
    CAN DRIVER Source File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf
    
    Author: Hamza Iqbal
*/

#include "can.h"
#include "can_config.h"

void CAN_Msp_Init(CAN_HandleTypeDef* can_h, uint8_t line)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(can_h->Instance == line)
    {
        /* Peripheral clock enable */
        __HAL_RCC_CAN1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        
        switch(line)
        {
            case CAN_line_1:
                /**CAN1 GPIO Configuration
                PB8     ------> CAN1_RX
                PB9     ------> CAN1_TX
                */
                GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
                GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
                break;
            case CAN_line_2:
                /**CAN2 GPIO Configuration
                PB6     ------> CAN2_RX
                PB7     ------> CAN2_TX
                */
                GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
                GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
                break;
            case CAN_line_3:
                /**CAN3 GPIO Configuration
                PB4     ------> CAN3_RX
                PB5     ------> CAN3_TX
                */
                GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
                GPIO_InitStruct.Alternate = GPIO_AF9_CAN3;
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
                break;
            default:
                /**CAN1 GPIO Configuration
                PB8     ------> CAN1_RX
                PB9     ------> CAN1_TX
                */
                GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
                GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
                break;
        }

        

    /* USER CODE BEGIN CAN1_MspInit 1 */

    /* USER CODE END CAN1_MspInit 1 */
    }
}

void CAN_Handler_Init(CAN_HandleTypeDef* can_h, uint8_t line)
{
    switch(line)
    {
        case CAN_line_1:
            can_h->Instance = CAN1;
            break;
        case CAN_line_2:
            can_h->Instance = CAN2;
            break;
        case CAN_line_3:
            can_h->Instance = CAN3;
            break;
        default:
            can_h->Instance = CAN1;
            break;
    }
    can_h->Init.Prescaler              = CAN_PRESCALER;
    can_h->Init.Mode                   = CAN_MODE;
    can_h->Init.SyncJumpWidth          = CAN_SYNC_JUMP_WIDTH;
    can_h->Init.TimeSeg1               = CAN_TIME_SEG_1;
    can_h->Init.TimeSeg2               = CAN_TIME_SEG_2;
    can_h->Init.TimeTriggeredMode      = CAN_TIME_TRIGGERED_MODE;
    can_h->Init.AutoBusOff             = CAN_AUTO_BUS_OFF;
    can_h->Init.AutoWakeUp             = CAN_AUTO_WAKEUP;
    can_h->Init.AutoRetransmission     = CAN_AUTO_RETRANSMISSION;
    can_h->Init.ReceiveFifoLocked      = CAN_RECEIVE_FIFO_LOCKED;
    can_h->Init.TransmitFifoPriority   = CAN_TRANSMIT_FIFO_PRIORITY;

    CAN_Msp_Init(can_h, line);

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
