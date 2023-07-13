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


uint8_t start_CAN()
{
    HAL_StatusTypeDef CAN_Status;

    CAN_Handler_Init(CAN1, CAN_line_1);
    CAN_Handler_Init(CAN2, CAN_line_2);
    CAN_Handler_Init(CAN3, CAN_line_3);



    CAN_Status = HAL_CAN_Start(CAN1);
    if (CAN_Status!= HAL_OK)
    {
        return FAIL;
    }
    CAN_Status = HAL_CAN_Start(CAN2);
    if (CAN_Status!= HAL_OK)
    {
        return FAIL;
    }
    CAN_Status = HAL_CAN_Start(CAN3);
    if (CAN_Status!= HAL_OK)
    {
        return FAIL;
    }
    return SUCCESS;
}

CAN_msg_t serializeCANMsg(uint32_t id, uint8_t len, uint8_t* data, uint8_t bus, CAN_HandleTypeDef* can_line)
{
    CAN_msg_t CAN_message;
    // Set up the message structure with ID and length of payload.
    CAN_message.id = id;
    CAN_message.len = len;
    CAN_message.bus = bus;
    uint8_t *data_temp;
    for (int i = 0; i < 8; i++) 
    {
        if (i < len)
        {
            data_temp = const_cast<uint8_t*>(data + i);
            CAN_message.data[i] = *data_temp;
        }
        else
        {
            CAN_message.data[i] = 0; // copies data to message, padding with 0s if length isn't 8
        }
    }
    switch(bus)
    {
        case CAN_line_1:
            CAN_message.CAN_handler = CAN1;
            break;
        case CAN_line_2:
            CAN_message.CAN_handler = CAN2;
            break;
        case CAN_line_3:
            CAN_message.CAN_handler = CAN3;
            break;
    }
    CAN_TxHeaderTypeDef* header = malloc(sizeof(CAN_TxHeaderTypeDef));
    header->StdId = CAN_STD_ID;
    header->ExtId = CAN_EXT_ID;
    header->IDE = CAN_ID_STD;
    header->RTR = CAN_RTR_data;
    header->DLC = 8;
    header->TransmitGlobalTime = DISABLE;
    
    return CAN_message;
}

int sendMessageCAN1(uint32_t id, uint8_t len, const uint8_t *data)
{
    CAN_msg_t message = serializeCANMsg(id, len, data, CAN_line_1, CAN1);
    uint32_t free_spots = HAL_CAN_GetTxMailboxesFreeLevel(message.CAN_handler);
    if(free_spots != 0)
    {
        HAL_StatusTypeDef bruh = HAL_CAN_AddTxMessage(message.CAN_handler, message.CAN_transmit_header, message.data, CAN_TX_MAILBOX0);
        if(bruh == HAL_OK)
        {
            return SUCCESS;
        }
        else
        {
            return FAIL;
        }
    }
    else
    {
        return FAIL;
    }
}

int sendMessageCAN2(uint32_t id, uint8_t len, const uint8_t *data)
{
    CAN_msg_t message = serializeCANMsg(id, len, data, CAN_line_2, CAN2);
    uint32_t free_spots = HAL_CAN_GetTxMailboxesFreeLevel(message.CAN_handler);
    if(free_spots != 0)
    {
        HAL_StatusTypeDef bruh = HAL_CAN_AddTxMessage(message.CAN_handler, message.CAN_transmit_header, message.data, CAN_TX_MAILBOX1);
        if(bruh == HAL_OK)
        {
            return SUCCESS;
        }
        else
        {
            return FAIL;
        }
    }
    else
    {
        return FAIL;
    }
}

int sendMessageCAN3(uint32_t id, uint8_t len, const uint8_t *data)
{
    CAN_msg_t message = serializeCANMsg(id, len, data, CAN_line_3, CAN3);
    uint32_t free_spots = HAL_CAN_GetTxMailboxesFreeLevel(message.CAN_handler);
    if(free_spots != 0)
    {
        HAL_StatusTypeDef bruh = HAL_CAN_AddTxMessage(message.CAN_handler, message.CAN_transmit_header, message.data, CAN_TX_MAILBOX2);
        if(bruh == HAL_OK)
        {
            return SUCCESS;
        }
        else
        {
            return FAIL;
        }
    }
    else
    {
        return FAIL;
    }
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
