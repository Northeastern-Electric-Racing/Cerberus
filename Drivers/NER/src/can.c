/*
    CAN DRIVER Source File
    Link to part Datasheet for reference:
    https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf

    Author: Hamza Iqbal
*/

#include <stdlib.h>
#include "can.h"
#include "can_config.h"

/*
 * These are the CAN Handlers they hold configuration information and whatnot you can change
 * this information by changing the Macros in can_config.h
 */
CAN_HandleTypeDef* can1;
CAN_HandleTypeDef* can2;
// CAN_HandleTypeDef* can3;

/* Used in the Queue implementation - you probably dont need to worry about it */
struct node
{
    can_msg_t msg;
    struct node* next;
};

/* This is a queue of messages that are waiting for processing by your application code */
struct msg_queue
{
    struct node* head;
    struct node* tail;
};

/*
 * This is a structure that holds the PIN configuration information, this configuration can also
 * be changed in can_config.h
 */
GPIO_InitTypeDef GPIO_InitStruct = {0};

/* This is a function to initialize low level parameters for the CAN lines */
static void can_msp_init(CAN_HandleTypeDef* can_h, uint8_t line)
{
    /* Peripheral clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    switch(line)
    {
        case CAN_LINE_1:
            /*
            * CAN1 GPIO Configuration
            * PB8     ------> CAN1_RX
            * PB9     ------> CAN1_TX
            */
            GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
            HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            break;
        case CAN_LINE_2:
            /*
            * CAN2 GPIO Configuration
            * PB6     ------> CAN2_RX
            * PB7     ------> CAN2_TX
            */
            GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
            HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            break;
        // case CAN_LINE_3:
        //     /*
        //     * CAN3 GPIO Configuration
        //     * PB4     ------> CAN3_RX
        //     * PB5     ------> CAN3_TX
        //     */
        //     GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
        //     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        //     GPIO_InitStruct.Pull = GPIO_NOPULL;
        //     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        //     GPIO_InitStruct.Alternate = GPIO_AF9_CAN3;
        //     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        //     break;
        default:
            /*
            * CAN1 GPIO Configuration
            * PB8     ------> CAN1_RX
            * PB9     ------> CAN1_TX
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

static void can_handler_init(CAN_HandleTypeDef* can_h, uint8_t line)
{
    switch(line)
    {
        case CAN_LINE_1:
            can_h = can1;
            break;
        case CAN_LINE_2:
            can_h = can2;
            break;
        // case CAN_LINE_3:
        //     can_h = can3;
        //     break;
        default:
            can_h = can1;
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

    can_msp_init(can_h, line);
}

HAL_StatusTypeDef can_init()
{
    HAL_StatusTypeDef can_status;

    can_handler_init(can1, CAN_LINE_1);
    can_handler_init(can2, CAN_LINE_2);
    // can_handler_init(can3, CAN_LINE_3);

    can_status = HAL_CAN_Start(can1);
    if (can_status!= HAL_OK)
    {
        return can_status;
    }
    can_status = HAL_CAN_Start(can2);
    if (can_status!= HAL_OK)
    {
        return can_status;
    }
    // can_status = HAL_CAN_Start(can3);
    // if (can_status!= HAL_OK)
    // {
    //     return can_status;
    // }

    HAL_CAN_ActivateNotification(can1, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_ActivateNotification(can2, CAN_IT_RX_FIFO1_MSG_PENDING);
    // HAL_CAN_ActivateNotification(can3, CAN_IT_RX_FIFO2_MSG_PENDING);
    return can_status;
}

CAN_StatusTypedef can_send_message(can_msg_t message)
{
    CAN_TxHeaderTypeDef* tx_header;
    uint32_t free_spots;

    tx_header = malloc(sizeof(CAN_TxHeaderTypeDef));
    tx_header->StdId = message.id;
    tx_header->ExtId = CAN_EXT_ID;
    tx_header->IDE = CAN_ID_STD;
    tx_header->RTR = CAN_RTR_DATA;
    tx_header->DLC = message.len;
    tx_header->TransmitGlobalTime = DISABLE;

    switch(message.line)
    {
        case CAN_LINE_1:
            free_spots = HAL_CAN_GetTxMailboxesFreeLevel(can1);
            if(free_spots != 0)
            {
                HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(can1, tx_header, message.data, CAN_TX_MAILBOX0);
                if(status == HAL_OK)
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
        case CAN_LINE_2:
            free_spots = HAL_CAN_GetTxMailboxesFreeLevel(can2);
            if(free_spots != 0)
            {
                HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(can2, tx_header, message.data, CAN_TX_MAILBOX1);
                if(status == HAL_OK)
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
        // case CAN_LINE_3:
        //     uint32_t free_spots = HAL_CAN_GetTxMailboxesFreeLevel(can3);
        //     if(free_spots != 0)
        //     {
        //         HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(can3, tx_header, message.data, CAN_TX_MAILBOX2);
        //         if(status == HAL_OK)
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
            free_spots = HAL_CAN_GetTxMailboxesFreeLevel(can1);
            if(free_spots != 0)
            {
                HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(can1, tx_header, message.data, CAN_TX_MAILBOX0);
                if(status == HAL_OK)
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

/* Function to add a node to the message queue it is automatically called in the interrupt triggered callback */
static void enqueue(struct msg_queue* queue, can_msg_t msg)
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

/* Removes and returns the front node of the queue */
static can_msg_t* dequeue(struct msg_queue* queue)
{
    if (queue->head == NULL)
    {
        return NULL;
    }

    can_msg_t *msg = malloc(sizeof(can_msg_t));
    *msg = queue->head->msg;
    struct node *old_head = queue->head;
    queue->head = queue->head->next;
    free(old_head);

    return msg;
}

// Interrupt triggered callback for CAN line 1
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef* rx_header = malloc(sizeof(CAN_RxHeaderTypeDef));
    can_msg_t new_msg;
    new_msg.line = CAN_LINE_1;
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, rx_header, new_msg.data);
    new_msg.len = rx_header->DLC;
    new_msg.id = rx_header->StdId;
    enqueue(can1_incoming, new_msg);
    free(rx_header);
}

// Interrupt triggered callback for CAN line 2
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef* rx_header = malloc(sizeof(CAN_RxHeaderTypeDef));
    can_msg_t new_msg;
    new_msg.line = CAN_LINE_2;
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, rx_header, new_msg.data);
    new_msg.len = rx_header->DLC;
    new_msg.id = rx_header->StdId;
    enqueue(can2_incoming, new_msg);
    free(rx_header);
}

/* Interrupt triggered callback for CAN line 3 */
// void HAL_CAN_RxFifo2MsgPendingCallback(CAN_HandleTypeDef *hcan)
// {
//     CAN_RxHeaderTypeDef* rx_header = malloc(sizeof(CAN_RxHeaderTypeDef));
//     CAN_msg_t new_msg;
//     new_msg.line = CAN_LINE_3;
//     HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO2, rx_header, new_msg.data);
//     new_msg.len = rx_header->DLC;
//     new_msg.id = rx_header->StdId;
//     enqueue(can3_incoming, new_msg);
//     free(rx_header);
// }

/* Retrieves the message at the front of the queue and dequeues */
can_msg_t *can_get_message(uint8_t line)
{
    can_msg_t *message;
    switch(line)
    {
        case CAN_LINE_1:
            message = dequeue(can1_incoming);
            break;
        case CAN_LINE_2:
            message = dequeue(can2_incoming);
            break;
        // case CAN_LINE_3:
        //     message = dequeue(can3_incoming);
        //     break;
        default:
            message = dequeue(can1_incoming);
            break;
    }
    return message;
}