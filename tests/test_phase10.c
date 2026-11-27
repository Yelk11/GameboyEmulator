#include <stdio.h>
#include <string.h>

#include "gb.h"
#include "unity.h"

static const char *phase10_rom = "build/phase10.gb";

void test_phase10_save_and_load_state(void)
{
    GB gb;
    char state_path[1024];
    FILE *fp;

    TEST_ASSERT_TRUE(gb_init(&gb));
    fp = fopen(phase10_rom, "wb");
    TEST_ASSERT_NOT_NULL(fp);
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));
    gb.cartridge.rom_name = malloc(strlen(phase10_rom) + 1u);
    TEST_ASSERT_NOT_NULL(gb.cartridge.rom_name);
    strcpy(gb.cartridge.rom_name, phase10_rom);

    gb.cpu->pc = 0x2345u;
    gb.cpu->sp = 0xDFFEu;
    gb.cpu->AF_reg.af = 0x1234u;
    gb.cpu->cycles = 12345u;
    gb.mmu->memory[0xC000u] = 0xA5u;
    gb.ppu->ly = 42u;
    gb.timer.tima = 0x77u;
    gb.joypad.action_buttons = 0x02u;

    TEST_ASSERT_TRUE(gb_save_state(&gb, 1u));

    gb.cpu->pc = 0x0100u;
    gb.cpu->sp = 0xFFFEu;
    gb.cpu->AF_reg.af = 0u;
    gb.cpu->cycles = 0u;
    gb.mmu->memory[0xC000u] = 0u;
    gb.ppu->ly = 0u;
    gb.timer.tima = 0u;
    gb.joypad.action_buttons = 0x0Fu;

    TEST_ASSERT_TRUE(gb_load_state(&gb, 1u));
    TEST_ASSERT_EQUAL_HEX16(0x2345u, gb.cpu->pc);
    TEST_ASSERT_EQUAL_HEX16(0xDFFEu, gb.cpu->sp);
    TEST_ASSERT_EQUAL_HEX16(0x1234u, gb.cpu->AF_reg.af);
    TEST_ASSERT_EQUAL_UINT32(12345u, gb.cpu->cycles);
    TEST_ASSERT_EQUAL_HEX8(0xA5u, gb.mmu->memory[0xC000u]);
    TEST_ASSERT_EQUAL_HEX8(42u, gb.ppu->ly);
    TEST_ASSERT_EQUAL_HEX8(0x77u, gb.timer.tima);
    TEST_ASSERT_EQUAL_HEX8(0x02u, gb.joypad.action_buttons);

    snprintf(state_path, sizeof(state_path), "build/phase10.state1");
    remove(state_path);
    remove(phase10_rom);
    gb_destroy(&gb);
}

void test_phase10_reject_wrong_rom(void)
{
    GB gb;
    GB other;
    char state_path[1024];
    FILE *fp;

    TEST_ASSERT_TRUE(gb_init(&gb));
    TEST_ASSERT_TRUE(gb_init(&other));
    fp = fopen("build/phase10a.gb", "wb");
    TEST_ASSERT_NOT_NULL(fp);
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));
    fp = fopen("build/phase10b.gb", "wb");
    TEST_ASSERT_NOT_NULL(fp);
    fputc(0x01u, fp);
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));

    gb.cartridge.rom_name = malloc(strlen("build/phase10a.gb") + 1u);
    TEST_ASSERT_NOT_NULL(gb.cartridge.rom_name);
    strcpy(gb.cartridge.rom_name, "build/phase10a.gb");
    TEST_ASSERT_TRUE(gb_save_state(&gb, 1u));

    other.mmu->cartridge.rom = malloc(1u);
    TEST_ASSERT_NOT_NULL(other.mmu->cartridge.rom);
    other.mmu->cartridge.rom[0] = 0x01u;
    other.mmu->cartridge.rom_size = 1u;
    other.cartridge.rom_name = malloc(strlen("build/phase10a.gb") + 1u);
    TEST_ASSERT_NOT_NULL(other.cartridge.rom_name);
    strcpy(other.cartridge.rom_name, "build/phase10a.gb");
    TEST_ASSERT_FALSE(gb_load_state(&other, 1u));

    snprintf(state_path, sizeof(state_path), "build/phase10a.state1");
    remove(state_path);
    remove("build/phase10a.gb");
    remove("build/phase10b.gb");
    gb_destroy(&gb);
    gb_destroy(&other);
}
