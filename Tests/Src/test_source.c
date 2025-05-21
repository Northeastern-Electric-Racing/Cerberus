#include "unity.h"
#include "mock_stub_functions.h"
#include "mock_debounce.h"
#include "mock_c_utils.h"
#include "mock_pdu.h"
#include "mock_state_machine.h"
#include "mock_mpu.h"
#include "mock_can_handler.h"
#include "pedals.h"

#include <stdbool.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

// A simple random test
void test_random(void) {
    debounce_Ignore();
    debounce_Ignore();
    TEST_ASSERT_EQUAL_INT(calc_pedal_faults(3.2, 1.2, 0.50, .10), true); 
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_random);
    return UNITY_END();
}