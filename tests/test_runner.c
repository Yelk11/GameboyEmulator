#include "unity.h"

void test_unity_assertions(void);
void test_cpu_executes_nop(void);
void test_cpu_step_fetches_nop(void);
void test_cpu_loads_bc_immediate(void);
void test_cpu_rejects_illegal_opcode(void);
void test_load_rom(void);
void test_load_rom_missing_file(void);

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
    RUN_TEST(test_load_rom);
    RUN_TEST(test_load_rom_missing_file);
    return UNITY_END();
}
