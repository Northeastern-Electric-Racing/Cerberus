#include "unity.h"
#include "mock_stub_functions.h"
#include "mock_debounce.h"
#include "mock_c_utils.h"
#include "mock_pdu.h"
#include "mock_mpu.h"
#include "mock_can_handler.h"
#include "pedals.h"
#include "state_machine.h"

#include <stdbool.h>
#include <stdlib.h>

mpu_t *mpu;
pdu_t *pdu;
dti_t *dti;

void setUp(void) {
    mpu = malloc(sizeof(mpu_t));
    pdu = malloc(sizeof(pdu_t));
    dti = malloc(sizeof(dti_t));
}

void tearDown(void) {
    free(mpu);
    free(pdu);
    free(dti);
}

// A simple random test
void test_random(void) {
    debounce_Ignore();
    debounce_Ignore();
    TEST_ASSERT_EQUAL_INT(calc_pedal_faults(3.2, 1.2, 0.50, .10), true); 

    write_fault_IgnoreAndReturn(0);
    TEST_ASSERT_EQUAL_INT(transition_functional_state(FAULTED, pdu, dti, mpu), 0);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_random);
    return UNITY_END();
}