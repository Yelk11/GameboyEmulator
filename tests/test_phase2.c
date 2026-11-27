#include "gb.h"
#include "unity.h"

void test_gb_initializes_subsystems(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    TEST_ASSERT_NOT_NULL(gb.cpu);
    TEST_ASSERT_NOT_NULL(gb.mmu);
    TEST_ASSERT_NOT_NULL(gb.ppu);
    TEST_ASSERT_FALSE(gb.cartridge.loaded);
    TEST_ASSERT_NULL(gb.cartridge.rom_name);
    gb_destroy(&gb);
}

void test_gb_reset_sets_post_boot_state(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb_reset(&gb);

    TEST_ASSERT_EQUAL_HEX16(0x01B0u, gb.cpu->AF_reg.af);
    TEST_ASSERT_EQUAL_HEX16(0x0013u, gb.cpu->BC_reg.bc);
    TEST_ASSERT_EQUAL_HEX16(0x00D8u, gb.cpu->DE_reg.de);
    TEST_ASSERT_EQUAL_HEX16(0x014Du, gb.cpu->HL_reg.hl);
    TEST_ASSERT_EQUAL_HEX16(0xFFFEu, gb.cpu->sp);
    TEST_ASSERT_EQUAL_HEX16(0x0100u, gb.cpu->pc);
    TEST_ASSERT_EQUAL_UINT32(0u, gb.cpu->cycles);
    TEST_ASSERT_TRUE(gb.options.skip_boot_rom);
    TEST_ASSERT_FALSE(gb.options.paused);
    TEST_ASSERT_EQUAL_UINT8(0u, gb.timer.div);
    TEST_ASSERT_EQUAL_UINT8(0u, gb.interrupt.ie);
    TEST_ASSERT_FALSE(gb.interrupt.ime);
    TEST_ASSERT_EQUAL_HEX8(0x91u, gb.mmu->memory[0xFF40u]);
    TEST_ASSERT_EQUAL_HEX8(0xFCu, gb.mmu->memory[0xFF47u]);

    gb_destroy(&gb);
}

void test_gb_tick_advances_scanline(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);

    TEST_ASSERT_EQUAL_UINT64(GB_T_CYCLES_PER_SCANLINE, gb.total_t_cycles);
    TEST_ASSERT_EQUAL_UINT8(1u, gb.ppu->ly);
    TEST_ASSERT_EQUAL_UINT16(0u, gb.ppu->line_dot);
    TEST_ASSERT_EQUAL_UINT32(GB_T_CYCLES_PER_SCANLINE, gb.ppu->frame_cycle);

    gb_destroy(&gb);
}

void test_gb_run_frame_advances_exact_cycles(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb_run_frame(&gb);

    TEST_ASSERT_EQUAL_UINT64(GB_T_CYCLES_PER_FRAME, gb.total_t_cycles);
    TEST_ASSERT_EQUAL_UINT64(1u, gb.frame_number);
    TEST_ASSERT_EQUAL_UINT8(0u, gb.ppu->ly);
    TEST_ASSERT_EQUAL_UINT16(0u, gb.ppu->line_dot);
    TEST_ASSERT_EQUAL_UINT32(0u, gb.ppu->frame_cycle);

    gb_destroy(&gb);
}
