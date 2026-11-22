#include "cpu.h"
#include "unity.h"

void test_cpu_executes_nop(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    TEST_ASSERT_TRUE(cpu_execute_opcode(&cpu, &mmu, 0x00));
}

void test_cpu_rejects_unimplemented_opcode(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    TEST_ASSERT_FALSE(cpu_execute_opcode(&cpu, &mmu, 0x01));
}
