
#include "mock_mpu.h"
#include "mock_pdu.h"
#include "test_state_machine.h"

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
void test_tansition_functional_state(void) {
    write_fault_IgnoreAndReturn(0);
    TEST_ASSERT_EQUAL_INT(transition_functional_state(FAULTED, pdu, dti, mpu), 0);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tansition_functional_state);
    return UNITY_END();
}