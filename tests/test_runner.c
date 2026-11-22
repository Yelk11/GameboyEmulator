#include "unity.h"

void test_unity_assertions(void);

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unity_assertions);
    return UNITY_END();
}
