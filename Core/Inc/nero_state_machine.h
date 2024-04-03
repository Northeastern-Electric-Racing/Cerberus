#ifndef NERO_STATE_MACHINE_H
#define NERO_STATE_MACHINE_H

typedef enum {
	NOT_DRIVING,
	REVERSE,
	PIT,
	EFFICIENCY,
	PERFORMANCE,
   NERO_ONLY,
	MAX_DRIVE_STATES
} drive_state_t;

typedef struct {
   drive_state_t nero_index;
   bool home_mode;
} nero_state_t;


/*
 * Attempts to increment the nero index and handles any exceptions 
*/
void increment_nero_index();

/*
 * Attempts to decrement the nero index and handles any exceptions
*/
void decrement_nero_index();

/*
 * Tells the statemachine to track up and down movements
*/
void set_home_mode();

/*
 * Tells NERO to select the current index if in home mode
*/
void select_nero_index();

#endif // NERO_STATE_MACHINE_H