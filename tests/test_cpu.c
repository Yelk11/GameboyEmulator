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
    TEST_ASSERT_EQUAL_HEX8(0x34, cpu.BC_reg.bytes.c);
    TEST_ASSERT_EQUAL_HEX8(0x12, cpu.BC_reg.bytes.b);
    TEST_ASSERT_EQUAL_UINT32(12, cpu.cycles);
}

void test_cpu_rejects_illegal_opcode(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    cpu.pc = 1;
    TEST_ASSERT_FALSE(cpu_execute_opcode(&cpu, &mmu, 0xd3));
}

void test_cpu_add_and_flags(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    cpu.pc = 0;
    cpu.AF_reg.bytes.a = 0x0Fu;
    mmu.memory[0] = 0x01u;
    TEST_ASSERT_TRUE(cpu_execute_opcode(&cpu, &mmu, 0xc6));
    TEST_ASSERT_EQUAL_HEX8(0x10u, cpu.AF_reg.bytes.a);
    TEST_ASSERT_TRUE((cpu.AF_reg.bytes.f & 0x20u) != 0u);
}

void test_cpu_conditional_jr(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    cpu.pc = 1;
    cpu.AF_reg.bytes.f = 0x80u;
    mmu.memory[1] = 0x05u;
    TEST_ASSERT_TRUE(cpu_execute_opcode(&cpu, &mmu, 0x20));
    TEST_ASSERT_TRUE(cpu.pc == 2u);
    TEST_ASSERT_EQUAL_UINT32(8u, cpu.cycles);
}

void test_cpu_call_and_return(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    cpu.pc = 1;
    cpu.sp = 0xFFFEu;
    mmu.memory[1] = 0x34u;
    mmu.memory[2] = 0x12u;
    TEST_ASSERT_TRUE(cpu_execute_opcode(&cpu, &mmu, 0xcd));
    TEST_ASSERT_EQUAL_HEX16(0x1234u, cpu.pc);
    TEST_ASSERT_EQUAL_HEX16(0xFFFCu, cpu.sp);

    mmu.memory[0xFFFCu] = 0x03u;
    mmu.memory[0xFFFDu] = 0x00u;
    TEST_ASSERT_TRUE(cpu_execute_opcode(&cpu, &mmu, 0xc9));
    TEST_ASSERT_EQUAL_HEX16(0x0003u, cpu.pc);
}

void test_cpu_cb_rotate(void)
{
    reg cpu = {0};
    MMU mmu = {0};

    cpu.pc = 1;
    cpu.BC_reg.bytes.b = 0x80u;
    mmu.memory[1] = 0x00u;
    TEST_ASSERT_TRUE(cpu_execute_opcode(&cpu, &mmu, 0xcb));
    TEST_ASSERT_EQUAL_HEX8(0x01u, cpu.BC_reg.bytes.b);
    TEST_ASSERT_TRUE((cpu.AF_reg.bytes.f & 0x10u) != 0u);
}
