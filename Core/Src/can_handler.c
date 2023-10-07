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

#include "can.h"
#include "can_config.h"
#include <stdlib.h>

#define MAX_MAP_SIZE 50 /* nodes */
#define CAN_MSG_QUEUE_SIZE 25 /* messages */
#define NUM_CALLBACKS 5 // Update when adding new callbacks

static osMessageQueueId_t can_inbound_queue;
static osMessageQueueId_t can_outbound_queue;
osThreadId_t route_can_incoming_handle;
const osThreadAttr_t route_can_incoming_attributes = { 
	.name = "RouteCanIncoming",
	.stack_size = 128 * 8,
	.priority = (osPriority_t)osPriorityAboveNormal4 
};

/* Used in the Queue implementation - you probably dont need to worry about it */
struct node {
	can_msg_t msg;
	struct node* next;
};

/* This is a queue of messages that are waiting for processing by your
 * application code */
struct msg_queue {
	struct node* head;
	struct node* tail;
};

/* Struct to couple function with message IDs */
typedef struct
{
	uint8_t id;
	void (*function)(void);
} function_info_t;

/* Hashmap node structure */
typedef struct hash_node_t {
	function_info_t info;
	struct hash_node_t* next;
} hash_node_t;

/* Hashmap structure */
typedef struct
{
	hash_node_t* array[MAX_MAP_SIZE];
} hash_map_t;


//TODO: Evaluate memory usage here
static function_info_t can_callbacks[] = { 
	//TODO: Implement MC_Update and other callbacks
	//{ .id = 0x2010, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2110, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2210, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2310, .function = (*MC_update)(can_msg_t) },
	//{ .id = 0x2410, .function = (*MC_update)(can_msg_t) } 
};

static struct hash_map_t callback_map;

// Initialize the hashmap
static void initializeHashMap(hash_map_t* hash_map)
{
	for (int i = 0; i < MAX_MAP_SIZE; i++)
		hash_map->array[i] = NULL;
}

// Hash function
static int hashFunction(int key)
{
	return key % MAX_MAP_SIZE;
}

/* Insert a function info into the hashmap */
static void insertFunction(hash_map_t* hash_map, int message_id, void (*function)(void))
{
	int index = hashFunction(message_id);

	hash_node_t* newNode = (hash_node_t*)malloc(sizeof(hash_node_t));
	if (!newNode) {
		// TODO: Send fault
	}

	newNode->info.id = message_id;
	newNode->info.function = function;
	newNode->next = NULL;

	if (hash_map->array[index] == NULL) {
		hash_map->array[index] = newNode;
	} else {
		hash_node_t* current = hash_map->array[index];
		while (current->next != NULL)
			current = current->next;
		current->next = newNode;
	}
}

/* Get the function associated with a message ID */
static void (*getFunction(hash_map_t* hash_map, int id))(void)
{
	int index = hashFunction(id);
	hash_node_t* current = hash_map->array[index];

	while (current != NULL) {
		if (current->info.id == id)
			return current->info.function;

		current = current->next;
	}

	return NULL; // Function not found
}

void can1_isr()
{
	//TODO: get CAN message

	/* Publish to Onboard Temp Queue */
	//osMessageQueuePut(can_inbound_queue, &can_msg, 0U, 0U);
}

void vRouteCanIncoming(void* pv_params)
{
	can_msg_t* message;
	osStatus_t status;
	void (*callback)(can_msg_t) = NULL;
	
	initializeHashMap(&callback_map);

	for (int i = 0; i < NUM_CALLBACKS; i++) {
		insertFunction(&callback_map, can_callbacks[i].id, can_callbacks[i].function);
	}

	can_inbound_queue = osMessageQueueNew(CAN_MSG_QUEUE_SIZE, sizeof(can_msg_t), NULL);

	// TODO: Link CAN1_ISR

	for(;;) {
		status = osMessageQueueGet(can_inbound_queue, &message, NULL, 0U);
		if (status != osOK) {
			//TODO: Trigger fault
		} else {
			callback = getFunction(&callback_map, message->id);
			if (callback == NULL) {
				//TODO: Trigger low priority error
			} else {
				callback(*message);
			}
		}
	}
}