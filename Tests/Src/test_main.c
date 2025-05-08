#include "unity.h"
#include "cerberus_test.h"
#include "pedals.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

// A simple random test
void test_random(void) {
    TEST_ASSERT_EQUAL_INT(15, 2); 
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_random);
    return UNITY_END();
}