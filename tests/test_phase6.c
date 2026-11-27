#include "gb.h"
#include "unity.h"

void test_phase6_scanline_timing(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);
    TEST_ASSERT_EQUAL_HEX8(1u, gb.ppu->ly);
    TEST_ASSERT_EQUAL_UINT16(0u, gb.ppu->line_dot);
    gb_destroy(&gb);
}

void test_phase6_background_tile(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0x91u;
    gb.mmu->memory[0xFF42u] = 0u;
    gb.mmu->memory[0xFF43u] = 0u;
    gb.mmu->memory[0xFF47u] = 0xE4u;
    gb.mmu->memory[0x9800u] = 1u;
    gb.mmu->memory[0x8010u] = 0xFFu;
    gb.mmu->memory[0x8011u] = 0u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);
    TEST_ASSERT_NOT_EQUAL_UINT32(0u, gb.ppu->framebuffer[0]);
    gb_destroy(&gb);
}

void test_phase6_vblank_interrupt(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb_tick(&gb, 144u * GB_T_CYCLES_PER_SCANLINE);
    TEST_ASSERT_EQUAL_HEX8(144u, gb.ppu->ly);
    TEST_ASSERT_EQUAL_HEX8(0x01u, gb.mmu->memory[0xFF0Fu] & 0x01u);
    TEST_ASSERT_EQUAL_INT(PPU_MODE_VBLANK, gb.ppu->mode);
    gb_destroy(&gb);
}

void test_phase6_lyc_stat(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF45u] = 1u;
    gb.mmu->memory[0xFF41u] = 0u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);
    TEST_ASSERT_TRUE((gb.mmu->memory[0xFF41u] & 0x04u) != 0u);
    gb_destroy(&gb);
}

void test_phase6_window_and_sprite_registers(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0xF3u;
    gb.mmu->memory[0xFF4Au] = 0u;
    gb.mmu->memory[0xFF4Bu] = 7u;
    gb.mmu->memory[0xFE00u] = 16u;
    gb.mmu->memory[0xFE01u] = 8u;
    gb.mmu->memory[0xFE02u] = 1u;
    gb.mmu->memory[0xFE03u] = 0u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);
    TEST_ASSERT_EQUAL_HEX8(1u, gb.ppu->ly);
    gb_destroy(&gb);
}
