/**
 * @file can_handler.h
 * @author Hamza Iqbal
 * @brief This CAN handler is meant to receive and properly route CAN messages and keep this task seperate
 *        the CAN driver.  The purpose of this is specifically to have a better way of routing the different
 *        messages.
 * @version 0.1
 * @date 2023-09-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#define NUM_CALLBACKS 5 //Update when adding new callbacks

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

struct FunctionInfo can_callbacks[] = {
                                            ({0x2010, void(*MC_update)(can_msg_t)}), 
                                            ({0x2110, void(*MC_update)(can_msg_t)}), 
                                            ({0x2210, void(*MC_update)(can_msg_t)}),
                                            ({0x2310, void(*MC_update)(can_msg_t)}),
                                            ({0x2410, void(*MC_update)(can_msg_t)})
                                      }

/* Struct to couple function with message IDs */
typedef struct 
{
    uint8_t messageID;
    void (*function)(void);
} FunctionInfo;

/* Hashmap node structure */
typedef struct HashNode 
{
    FunctionInfo info;
    struct HashNode* next;
} HashNode;

/* Hashmap structure */
typedef struct 
{
    HashNode* array[MAX_MAP_SIZE];
} HashMap;

/* These are the queues for each CAN line */
struct msg_queue* can1_incoming;
struct msg_queue* can2_incoming;
//struct msg_queue* can3_incoming;
struct msg_queue* can1_outgoing;
struct msg_queue* can2_outgoing;
//struct msg_queue* can3_outgoing;


void can_handler_init();

static void enqueue(struct msg_queue* queue, can_msg_t msg);

static can_msg_t* dequeue(struct msg_queue* queue);



#endif