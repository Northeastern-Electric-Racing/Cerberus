/**
 * @file can.h
 * @author Hamza Iqbal (iqbal.ha@northeastern.edu)
 * @brief CAN Driver
 *        HOW TO USE THIS DRIVER:
 *          First use the start_CAN function to enable all the CAN lines
 *          You can send messages using the send_message_CAN function, this function requires a CAN_msg_t struct as the parameter.
 *          Configure a CAN_msg_t struct with the ID of the message, the Data, the length and what CAN line you want to transmit on.
 *
 *          This driver is interrupt based and as incoming messages are received they are added to a queue per can line
 *          You can receive the front of the queue by using the get_message_CAN function this will return a CAN_msg_t struct containing all the useful information
 *
 *          In the src file for this driver you will notice a lot of commented out sections pertaining to a third CAN line, ignore these for now as the MPU
 *          can only support 2 CAN lines at the moment.  This will come in handy for a third CAN line if we choose to implement one later or for a different system.
 * @date 2023-06-14
 *
 * https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf
 * CAN Documentation can be found in pages 87-113
 */

#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "can_config.h"

/* CAN Message type this holds all useful CAN information */
typedef struct
{
    uint8_t line;
    uint32_t id;
    uint8_t len;
    uint8_t data[8];
} can_msg_t;

/* These are the queues for each CAN line */
struct msg_queue* can1_incoming;
struct msg_queue* can2_incoming;
// msg_queue* can3_incoming;


/* Use this when referencing CAN lines when sending a message */
enum
{
    CAN_LINE_1 = 1,
    CAN_LINE_2 = 2,
    //CAN_LINE_3 = 3
};

/* Fault codes */
typedef enum
{
    MSG_SENT = 1,
    MSG_FAIL = 2,
    BUFFER_FULL = 3
} CAN_StatusTypedef;

/**
 * @brief Initializes the can handlers for each line
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef can_init();

/**
 * @brief Sends a CAN message on the line specified in the CAN_msg_t struct
 *
 * @param message A CAN_msg_t struct containing the information you want to send
 * @return uint8_t Error code
 */
CAN_StatusTypedef can_send_message(can_msg_t message);

/**
 * @brief Returns a message from the front of the queue
 *
 * @param line The CAN line that you want to get the message from
 * @return CAN_msg_t Struct containing important message info
 */
can_msg_t *can_get_message(uint8_t line);


#endif
