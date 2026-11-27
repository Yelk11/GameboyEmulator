#include "gb.h"
#include "unity.h"

void test_phase5_timer_overflow(void)
{
    GB gb;
    TEST_ASSERT_TRUE(gb_init(&gb));
    mmu_write_byte(gb.mmu, 0xFF05u, 0xFFu);
    mmu_write_byte(gb.mmu, 0xFF06u, 0x80u);
    mmu_write_byte(gb.mmu, 0xFF07u, 0x05u);
    gb_tick(&gb, 16u);
    TEST_ASSERT_EQUAL_HEX8(0x80u, gb.mmu->memory[0xFF05u]);
    TEST_ASSERT_TRUE((gb.mmu->memory[0xFF0Fu] & 0x04u) != 0u);
    gb_destroy(&gb);
}

void test_phase5_joypad_interrupt(void)
{
    GB gb;
    TEST_ASSERT_TRUE(gb_init(&gb));
    joypad_press(&gb.joypad, 0u, true);
    gb_tick(&gb, 1u);
    TEST_ASSERT_TRUE((gb.mmu->memory[0xFF0Fu] & 0x10u) != 0u);
    gb_destroy(&gb);
}

void test_phase5_dma_transfer(void)
{
    GB gb;
    TEST_ASSERT_TRUE(gb_init(&gb));
    mmu_write_byte(gb.mmu, 0xC000u, 0xA5u);
    mmu_write_byte(gb.mmu, 0xFF46u, 0xC0u);
    gb_tick(&gb, 640u);
    TEST_ASSERT_EQUAL_HEX8(0xA5u, gb.mmu->memory[0xFE00u]);
    TEST_ASSERT_FALSE(gb.dma.active);
    gb_destroy(&gb);
}

void test_phase5_interrupt_step(void)
{
    GB gb;
    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.cpu->ime = true;
    gb.mmu->memory[0xFFFFu] = 0x01u;
    gb.mmu->memory[0xFF0Fu] = 0x01u;
    gb.cpu->sp = 0xFFFEu;
    gb.cpu->pc = 0x0100u;
    TEST_ASSERT_EQUAL_UINT32(20u, interrupt_step(&gb));
    TEST_ASSERT_EQUAL_HEX16(0x0040u, gb.cpu->pc);
    TEST_ASSERT_FALSE(gb.cpu->ime);
    TEST_ASSERT_EQUAL_HEX8(0u, gb.mmu->memory[0xFF0Fu]);
    gb_destroy(&gb);
}

void test_phase5_serial_transfer(void)
{
    GB gb;
    TEST_ASSERT_TRUE(gb_init(&gb));
    mmu_write_byte(gb.mmu, 0xFF01u, 0x42u);
    mmu_write_byte(gb.mmu, 0xFF02u, 0x81u);
    gb_tick(&gb, 512u);
    TEST_ASSERT_EQUAL_HEX8(0x42u, gb.mmu->memory[0xFF01u]);
    TEST_ASSERT_EQUAL_HEX8(0x01u, gb.mmu->memory[0xFF02u]);
    gb_destroy(&gb);
}
