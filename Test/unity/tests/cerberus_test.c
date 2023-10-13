#include "unity.h"
#include "cerberus_test.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_can_handler);
    return UNITY_END();
}