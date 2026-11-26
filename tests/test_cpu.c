#include "cpu.h"
#include "unity.h"

void test_cpu_executes_nop(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    TEST_ASSERT_TRUE(cpu_execute_opcode(&cpu, &mmu, 0x00));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.cycles);
}

void test_cpu_step_fetches_nop(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    mmu.memory[0] = 0x00;

    TEST_ASSERT_TRUE(cpu_step(&cpu, &mmu));
    TEST_ASSERT_EQUAL_UINT16(1, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(4, cpu.cycles);
}

void test_cpu_loads_bc_immediate(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    mmu.memory[0] = 0x34;
    mmu.memory[1] = 0x12;

    TEST_ASSERT_TRUE(cpu_execute_opcode(&cpu, &mmu, 0x01));
    TEST_ASSERT_EQUAL_HEX8(0x34, cpu.c);
    TEST_ASSERT_EQUAL_HEX8(0x12, cpu.b);
    TEST_ASSERT_EQUAL_UINT32(12, cpu.cycles);
}

void test_cpu_rejects_illegal_opcode(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    cpu.pc = 1;
    TEST_ASSERT_FALSE(cpu_execute_opcode(&cpu, &mmu, 0xd3));
}
