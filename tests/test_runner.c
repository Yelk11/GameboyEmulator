#include "unity.h"

void test_unity_assertions(void);
void test_cpu_executes_nop(void);
void test_cpu_step_fetches_nop(void);
void test_cpu_loads_bc_immediate(void);
void test_cpu_rejects_illegal_opcode(void);

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
    RUN_TEST(test_cpu_executes_nop);
    RUN_TEST(test_cpu_step_fetches_nop);
    RUN_TEST(test_cpu_loads_bc_immediate);
    RUN_TEST(test_cpu_rejects_illegal_opcode);
    return UNITY_END();
}
