#include <unity/unity.h>

void setUp(void) {
}

void tearDown(void) {}

void test_function_something(void) {
    TEST_ABORT();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_function_something);
    return UNITY_END();
}
