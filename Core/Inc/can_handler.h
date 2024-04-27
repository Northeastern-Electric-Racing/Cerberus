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

void can1_callback(CAN_HandleTypeDef* hcan);

void vCanDispatch(void* pv_params);
extern osThreadId_t can_dispatch_handle;
extern const osThreadAttr_t can_dispatch_attributes;

int8_t queue_can_msg(can_msg_t msg);
void init_can1(CAN_HandleTypeDef* hcan);

#endif
