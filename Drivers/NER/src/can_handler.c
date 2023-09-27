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

    HashMap callback_map;
    initializeHashMap(&callback_map);

    for(int i = 0; i < NUM_CALLBACKS; i++)
    {
        insertFunction(&calllback_map, can_callbacks[i].messageID, can_callbacks[i].function);
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

// Initialize the hashmap
void initializeHashMap(HashMap* hashMap) {
    for (int i = 0; i < MAX_MAP_SIZE; i++)
        hashMap->array[i] = NULL;
}

// Hash function
int hashFunction(int key) {
    return key % MAX_MAP_SIZE;
}

// Insert a function info into the hashmap
void insertFunction(HashMap* hashMap, int messageID, void (*function)(void)) {
    int index = hashFunction(messageID);

    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    if (!newNode) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    newNode->info.messageID = messageID;
    newNode->info.function = function;
    newNode->next = NULL;

    if (hashMap->array[index] == NULL) {
        hashMap->array[index] = newNode;
    } else {
        HashNode* current = hashMap->array[index];
        while (current->next != NULL)
            current = current->next;
        current->next = newNode;
    }
}

// Get the function associated with a message ID
void (*getFunction(HashMap* hashMap, int messageID))(void) {
    int index = hashFunction(messageID);
    HashNode* current = hashMap->array[index];

    while (current != NULL) {
        if (current->info.messageID == messageID)
            return current->info.function;

        current = current->next;
    }

    return NULL;  // Function not found
}
