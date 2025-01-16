#include "pdu.h"

// TODO: replace this temp value with real value
#define CONTROL_CANID_FANBATTBOX 0xA

extern osThreadId_t control_handle;
extern const osThreadAttr_t control_attributes;

typedef struct {
	pdu_t *pdu;
} control_args_t;

void vEval_fanbattbox_state(void *param);

void control_fanbattbox_record(can_msg_t args);

int eval_fanbattbox_state();