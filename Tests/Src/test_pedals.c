
#include "mock_stub_functions.h"
#include "mock_debounce.h"
#include "mock_c_utils.h"
#include "mock_pdu.h"
#include "mock_mpu.h"
#include "mock_can_handler.h"
#include "test_pedals.h"

void setUp(void) {

}

void tearDown(void) {
}

void test_calc_pedal_faults() {
    debounce_Ignore();
    debounce_Ignore();
    TEST_ASSERT_EQUAL_INT(calc_pedal_faults(3.2, 1.2, 0.50, .10), false); 

    debounce_Ignore();
    debounce_Ignore();
    TEST_ASSERT_EQUAL_INT(calc_pedal_faults(3.2, 1.2, 0.50, .30), false); 
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_calc_pedal_faults);
    return UNITY_END();
}