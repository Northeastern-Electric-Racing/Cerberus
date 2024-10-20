/**
 * @file can_handler.h
 * @author Hamza Iqbal
 * @brief This CAN handler is meant to receive and properly route CAN messages
 * and keep this task seperate the CAN driver.  The purpose of this is
 * specifically to have a better way of routing the different messages.
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include "can.h"
#include "cmsis_os.h"
#include "dti.h"

/**
 * @brief Callback to be called when a message is received on CAN line 1.
 * 
 * @param hcan Pointer to struct representing CAN hardware.
 */
void can1_callback(CAN_HandleTypeDef *hcan);

/**
 * @brief Place a CAN message in a queue.
 * 
 * @param msg CAN message to be sent.
 * @return int8_t Error code.
 */
int8_t queue_can_msg(can_msg_t msg);

/**
 * @brief Initialize CAN line 1.
 * 
 * @param hcan Pointer to struct representing CAN hardware.
 */
void init_can1(CAN_HandleTypeDef *hcan);

/**
 * @brief Task for sending CAN messages.
 * 
 * @param pv_params CAN_HandleTypeDef for the CAN line that messages will be sent out on.
 */
void vCanDispatch(void *pv_params);
extern osThreadId_t can_dispatch_handle;
extern const osThreadAttr_t can_dispatch_attributes;

/**
 * @brief Task for processing received can messages.
 * 
 * @param pv_params A can_receive_args_t*.
 */
void vCanReceive(void *pv_params);
extern osThreadId_t can_receive_thread;
extern const osThreadAttr_t can_receive_attributes;

#endif
