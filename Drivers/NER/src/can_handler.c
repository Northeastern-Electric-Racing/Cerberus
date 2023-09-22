/**
 * @file can_handler.c
 * @author Hamza Iqbal
 * @brief Source file for CAN handler.  
 * @version 0.1
 * @date 2023-09-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdlib.h>
#include "can.h"
#include "can_config.h"

/* Initializes the CAN handler */
void can_handler_init()
{
    // Assign memory to the queues
    can1_incoming = malloc(sizeof(msg_queue));
    can2_incoming = malloc(sizeof(msg_queue));
    can1_outgoing = malloc(sizeof(msg_queue));
    can2_outgoing = malloc(sizeof(msg_queue));
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
