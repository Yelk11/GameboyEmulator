#include "gb.h"
#include "unity.h"

void test_phase6_scanline_timing(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0x80u;
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
    gb.mmu->memory[0xFF40u] = 0x80u;
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
    gb.mmu->memory[0xFF40u] = 0x80u;
    gb.mmu->memory[0xFF45u] = 1u;
    gb.mmu->memory[0xFF41u] = 0u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);
    TEST_ASSERT_TRUE((gb.mmu->memory[0xFF41u] & 0x04u) != 0u);
    gb_destroy(&gb);
}

void test_phase6_stat_mode_interrupt_edge(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0x80u;
    gb.mmu->memory[0xFF41u] = 0x20u;
    gb_tick(&gb, 4u);
    TEST_ASSERT_TRUE((gb.mmu->memory[0xFF0Fu] & 0x02u) != 0u);

    gb.mmu->memory[0xFF0Fu] &= (uint8_t)~0x02u;
    gb_tick(&gb, 4u);
    TEST_ASSERT_EQUAL_HEX8(0u, gb.mmu->memory[0xFF0Fu] & 0x02u);
    gb_destroy(&gb);
}

void test_phase6_stat_hblank_interrupt(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0x80u;
    gb.mmu->memory[0xFF41u] = 0x08u;
    gb.ppu->line_dot = 248u;
    gb.ppu->mode = PPU_MODE_TRANSFER;
    gb.mmu->ppu_mode = PPU_MODE_TRANSFER;
    gb_tick(&gb, 4u);
    TEST_ASSERT_EQUAL_INT(PPU_MODE_HBLANK, gb.ppu->mode);
    TEST_ASSERT_TRUE((gb.mmu->memory[0xFF0Fu] & 0x02u) != 0u);
    gb_destroy(&gb);
}

void test_phase6_stat_lyc_interrupt(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0x80u;
    gb.mmu->memory[0xFF41u] = 0x40u;
    gb.mmu->memory[0xFF45u] = 1u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);

    TEST_ASSERT_TRUE((gb.mmu->memory[0xFF41u] & 0x04u) != 0u);
    TEST_ASSERT_TRUE((gb.mmu->memory[0xFF0Fu] & 0x02u) != 0u);
    gb_destroy(&gb);
}

void test_phase6_lcd_off_resets_ly_and_mode(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0u;
    gb.ppu->ly = 42u;
    gb.ppu->line_dot = 120u;
    gb.ppu->frame_cycle = 1234u;
    gb_tick(&gb, 4u);
    TEST_ASSERT_EQUAL_HEX8(0u, gb.ppu->ly);
    TEST_ASSERT_EQUAL_UINT16(0u, gb.ppu->line_dot);
    TEST_ASSERT_EQUAL_UINT32(0u, gb.ppu->frame_cycle);
    TEST_ASSERT_EQUAL_INT(PPU_MODE_HBLANK, gb.ppu->mode);
    gb_destroy(&gb);
}

void test_phase6_bg_disabled_uses_color_zero_and_keeps_sprites(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0x82u;
    gb.mmu->memory[0xFF47u] = 0xE4u;
    gb.mmu->memory[0xFF48u] = 0x0Cu;
    gb.mmu->memory[0xFE00u] = 16u;
    gb.mmu->memory[0xFE01u] = 8u;
    gb.mmu->memory[0xFE02u] = 1u;
    gb.mmu->memory[0x8010u] = 0x80u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);

    TEST_ASSERT_EQUAL_HEX32(0xFF000000u, gb.ppu->framebuffer[0]);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, gb.ppu->framebuffer[1]);
    gb_destroy(&gb);
}

void test_phase6_dmg_sprite_priority_uses_x_then_oam_order(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0x82u;
    gb.mmu->memory[0xFF48u] = 0x04u;
    gb.mmu->memory[0xFF49u] = 0x08u;
    gb.mmu->memory[0xFE00u] = 16u;
    gb.mmu->memory[0xFE01u] = 19u;
    gb.mmu->memory[0xFE02u] = 1u;
    gb.mmu->memory[0xFE03u] = 0x10u;
    gb.mmu->memory[0xFE04u] = 16u;
    gb.mmu->memory[0xFE05u] = 19u;
    gb.mmu->memory[0xFE06u] = 2u;
    gb.mmu->memory[0xFE08u] = 16u;
    gb.mmu->memory[0xFE09u] = 20u;
    gb.mmu->memory[0xFE0Au] = 3u;
    gb.mmu->memory[0x8010u] = 0x40u;
    gb.mmu->memory[0x8020u] = 0x40u;
    gb.mmu->memory[0x8030u] = 0x80u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);

    TEST_ASSERT_EQUAL_HEX32(0xFF555555u,
                            gb.ppu->framebuffer[12u]);
    gb_destroy(&gb);
}

void test_phase6_obj_priority_uses_bg_color_index(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0x93u;
    gb.mmu->memory[0xFF47u] = 0u;
    gb.mmu->memory[0xFF48u] = 0x0Cu;
    gb.mmu->memory[0x9800u] = 1u;
    gb.mmu->memory[0x8010u] = 0x80u;
    gb.mmu->memory[0xFE00u] = 16u;
    gb.mmu->memory[0xFE01u] = 8u;
    gb.mmu->memory[0xFE02u] = 2u;
    gb.mmu->memory[0xFE03u] = 0x80u;
    gb.mmu->memory[0x8020u] = 0x80u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, gb.ppu->framebuffer[0]);
    gb_destroy(&gb);
}

void test_phase6_window_line_counter_pauses_when_hidden(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0xF1u;
    gb.mmu->memory[0xFF47u] = 0xE4u;
    gb.mmu->memory[0xFF4Au] = 0u;
    gb.mmu->memory[0xFF4Bu] = 7u;
    gb.mmu->memory[0x9C00u] = 1u;
    gb.mmu->memory[0x8010u] = 0xFFu;
    gb.mmu->memory[0x8013u] = 0xFFu;
    gb.mmu->memory[0x8014u] = 0xFFu;
    gb.mmu->memory[0x8015u] = 0xFFu;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);
    TEST_ASSERT_EQUAL_UINT8(1u, gb.ppu->window_line);

    gb.mmu->memory[0xFF4Bu] = 167u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);
    TEST_ASSERT_EQUAL_UINT8(1u, gb.ppu->window_line);

    gb.mmu->memory[0xFF4Bu] = 7u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);
    TEST_ASSERT_EQUAL_HEX32(0xFF555555u,
                            gb.ppu->framebuffer[2u * GB_FRAMEBUFFER_WIDTH]);
    TEST_ASSERT_EQUAL_UINT8(2u, gb.ppu->window_line);
    gb_destroy(&gb);
}

void test_phase6_signed_tile_index_uses_9000_relative_base(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.mmu->memory[0xFF40u] = 0x81u;
    gb.mmu->memory[0xFF47u] = 0xE4u;
    gb.mmu->memory[0x9800u] = 0xA1u;
    gb.mmu->memory[0x8A10u] = 0x80u;
    gb_tick(&gb, GB_T_CYCLES_PER_SCANLINE);

    TEST_ASSERT_EQUAL_HEX32(0xFFAAAAAAu, gb.ppu->framebuffer[0]);
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
