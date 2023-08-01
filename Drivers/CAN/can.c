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
            // case CAN_line_3:
            //     /**CAN3 GPIO Configuration
            //     PB4     ------> CAN3_RX
            //     PB5     ------> CAN3_TX
            //     */
            //     GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
            //     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            //     GPIO_InitStruct.Pull = GPIO_NOPULL;
            //     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            //     GPIO_InitStruct.Alternate = GPIO_AF9_CAN3;
            //     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            //     break;
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
        // case CAN_line_3:
        //     can_h->Instance = CAN3;
        //     break;
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


HAL_StatusTypeDef start_CAN()
{
    HAL_StatusTypeDef CAN_Status;

    CAN_Handler_Init(CAN1, CAN_line_1);
    CAN_Handler_Init(CAN2, CAN_line_2);
    CAN_Handler_Init(CAN3, CAN_line_3);



    CAN_Status = HAL_CAN_Start(CAN1);
    if (CAN_Status!= HAL_OK)
    {
        return CAN_Status;
    }
    CAN_Status = HAL_CAN_Start(CAN2);
    if (CAN_Status!= HAL_OK)
    {
        return CAN_Status;
    }
    // CAN_Status = HAL_CAN_Start(CAN3);
    // if (CAN_Status!= HAL_OK)
    // {
    //     return CAN_Status;
    // }
    
    HAL_CAN_ActivateNotification(CAN1, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_ActivateNotification(CAN2, CAN_IT_RX_FIFO1_MSG_PENDING);
    // HAL_CAN_ActivateNotification(CAN3, CAN_IT_RX_FIFO2_MSG_PENDING);
    return CAN_Status;
}


uint8_t send_message_CAN(CAN_msg_t message)
{
    switch(message.line)
    {
        case CAN_line_1:
            CAN_TxHeaderTypeDef* tx_header = malloc(sizeof(CAN_TxHeaderTypeDef));
            tx_header->StdId = message.id;
            tx_header->ExtId = CAN_EXT_ID;
            tx_header->IDE = CAN_ID_STD;
            tx_header->RTR = CAN_RTR_data;
            tx_header->DLC = message.len;
            tx_header->TransmitGlobalTime = DISABLE;
            uint32_t free_spots = HAL_CAN_GetTxMailboxesFreeLevel(CAN1);
            if(free_spots != 0)
            {
                HAL_StatusTypeDef bruh = HAL_CAN_AddTxMessage(CAN1, tx_header, message.data, CAN_TX_MAILBOX0);
                if(bruh == HAL_OK)
                {
                    free(tx_header);
                    return MSG_SENT;
                }
                else
                {
                    free(tx_header);
                    return MSG_FAIL;
                }
                }
            else
            {
                free(tx_header);
                return BUFFER_FULL;
            }
            break;
        case CAN_line_2:
            CAN_TxHeaderTypeDef* tx_header = malloc(sizeof(CAN_TxHeaderTypeDef));
            tx_header->StdId = message.id;
            tx_header->ExtId = CAN_EXT_ID;
            tx_header->IDE = CAN_ID_STD;
            tx_header->RTR = CAN_RTR_data;
            tx_header->DLC = message.len;
            tx_header->TransmitGlobalTime = DISABLE;
            uint32_t free_spots = HAL_CAN_GetTxMailboxesFreeLevel(CAN2);
            if(free_spots != 0)
            {
                HAL_StatusTypeDef bruh = HAL_CAN_AddTxMessage(CAN2, tx_header, message.data, CAN_TX_MAILBOX1);
                if(bruh == HAL_OK)
                {
                    free(tx_header);
                    return MSG_SENT;
                }
                else
                {
                    free(tx_header);
                    return MSG_FAIL;
                }
                }
            else
            {
                free(tx_header);
                return BUFFER_FULL;
            }
            break;
        // case CAN_line_3:
        //     CAN_TxHeaderTypeDef* tx_header = malloc(sizeof(CAN_TxHeaderTypeDef));
        //     tx_header->StdId = message.id;
        //     tx_header->ExtId = CAN_EXT_ID;
        //     tx_header->IDE = CAN_ID_STD;
        //     tx_header->RTR = CAN_RTR_data;
        //     tx_header->DLC = message.len;
        //     tx_header->TransmitGlobalTime = DISABLE;
        //     uint32_t free_spots = HAL_CAN_GetTxMailboxesFreeLevel(CAN3);
        //     if(free_spots != 0)
        //     {
        //         HAL_StatusTypeDef bruh = HAL_CAN_AddTxMessage(CAN3, tx_header, message.data, CAN_TX_MAILBOX2);
        //         if(bruh == HAL_OK)
        //         {
        //             free(tx_header);
        //             return SUCCESS;
        //         }
        //         else
        //         {
        //             free(tx_header);
        //             return FAIL;
        //         }
        //         }
        //     else
        //     {
        //         free(tx_header);
        //         return FAIL;
        //     }
        //     break;
        default:
            CAN_TxHeaderTypeDef* tx_header = malloc(sizeof(CAN_TxHeaderTypeDef));
            tx_header->StdId = message.id;
            tx_header->ExtId = CAN_EXT_ID;
            tx_header->IDE = CAN_ID_STD;
            tx_header->RTR = CAN_RTR_data;
            tx_header->DLC = message.len;
            tx_header->TransmitGlobalTime = DISABLE;
            uint32_t free_spots = HAL_CAN_GetTxMailboxesFreeLevel(CAN1);
            if(free_spots != 0)
            {
                HAL_StatusTypeDef bruh = HAL_CAN_AddTxMessage(CAN1, tx_header, message.data, CAN_TX_MAILBOX0);
                if(bruh == HAL_OK)
                {
                    free(tx_header);
                    return MSG_SENT;
                }
                else
                {
                    free(tx_header);
                    return MSG_FAIL;
                }
                }
            else
            {
                free(tx_header);
                return BUFFER_FULL;
            }
            break;
    }

}

void enqueue(struct queue* queue, CAN_msg_t msg) 
{
  struct node *new_node = malloc(sizeof(struct node));
  new_node->msg = msg;
  new_node->next = NULL;

  if (queue->head == NULL) 
  {
    queue->head = new_node;
    queue->tail = new_node;
  } 
  else 
  {
    queue->tail->next = new_node;
    queue->tail = new_node;
  }
}

CAN_msg_t dequeue(struct queue* queue)
{
    if (queue->head == NULL) 
    {
        return -1;
    }

    int msg = queue->head->data;
    struct node *old_head = queue->head;
    queue->head = queue->head->next;
    free(old_head);

    return msg;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN1)
{
    CAN_RxHeaderTypeDef* rx_header = malloc(sizeof(CAN_RxHeaderTypeDef));
    CAN_msg_t new_msg;
    new_msg.line = CAN_line_1;
    HAL_CAN_GetRxMessage(CAN1, CAN_RX_FIFO0, rx_header, new_msg.data);
    new_msg.len = rx_header->DLC;
    new_msg.id = rx_header->StdId;
    enqueue(CAN1_incoming, new_msg);
    free(rx_header);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN2)
{
    CAN_RxHeaderTypeDef* rx_header = malloc(sizeof(CAN_RxHeaderTypeDef));
    CAN_msg_t new_msg;
    new_msg.line = CAN_line_2;
    HAL_CAN_GetRxMessage(CAN2, CAN_RX_FIFO1, rx_header, new_msg.data);
    new_msg.len = rx_header->DLC;
    new_msg.id = rx_header->StdId;
    enqueue(CAN2_incoming, new_msg);
    free(rx_header);
}

// void HAL_CAN_RxFifo2MsgPendingCallback(CAN2)
// {
//     CAN_RxHeaderTypeDef* rx_header = malloc(sizeof(CAN_RxHeaderTypeDef));
//     CAN_msg_t new_msg;
//     new_msg.line = CAN_line_3;
//     HAL_CAN_GetRxMessage(CAN3, CAN_RX_FIFO2, rx_header, new_msg.data);
//     new_msg.len = rx_header->DLC;
//     new_msg.id = rx_header->StdId;
//     enqueue(CAN3_incoming, new_msg);
//     free(rx_header);
// }

CAN_msg_t get_message_CAN(uint8_t line)
{
    CAN_msg_t message;
    switch(line)
    {
        case CAN_line_1:
            message = dequeue(CAN1_incoming);
            break;
        case CAN_line_2:
            message = dequeue(CAN2_incoming);
            break;
        // case CAN_line_3:
        //     message = dequeue(CAN3_incoming);
        //     break;
        default:
            message = dequeue(CAN1_incoming);
            break;
    }
    return message;
}