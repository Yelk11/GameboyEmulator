#include <stdio.h>
#include <string.h>

#include "gb.h"
#include "unity.h"

static const char *phase11_rom = "build/phase11.gb";

static MMU *load_cgb_rom(void)
{
    FILE *fp = fopen(phase11_rom, "wb");
    uint8_t rom[0x150];
    MMU *mmu;

    TEST_ASSERT_NOT_NULL(fp);
    memset(rom, 0, sizeof(rom));
    rom[0x143u] = 0xC0u;
    rom[0x147u] = 0x00u;
    TEST_ASSERT_EQUAL_UINT32(sizeof(rom), (uint32_t)fwrite(rom, 1, sizeof(rom), fp));
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));
    mmu = mmu_init();
    TEST_ASSERT_NOT_NULL(mmu);
    TEST_ASSERT_TRUE(mmu_load_rom(mmu, phase11_rom));
    return mmu;
}

void test_phase11_cgb_header_and_vram(void)
{
    MMU *mmu = load_cgb_rom();

    TEST_ASSERT_TRUE(mmu_is_cgb(mmu));
    mmu_write_byte(mmu, 0xFF4Fu, 1u);
    mmu_write_byte(mmu, 0x8000u, 0xA5u);
    TEST_ASSERT_EQUAL_HEX8(0xA5u, mmu_read_byte(mmu, 0x8000u));
    mmu_write_byte(mmu, 0xFF4Fu, 0u);
    TEST_ASSERT_EQUAL_HEX8(0u, mmu_read_byte(mmu, 0x8000u));

    mmu_destroy(mmu);
    remove(phase11_rom);
}

void test_phase11_wram_and_palette(void)
{
    MMU *mmu = load_cgb_rom();

    mmu_write_byte(mmu, 0xFF70u, 2u);
    mmu_write_byte(mmu, 0xD123u, 0x5Au);
    TEST_ASSERT_EQUAL_HEX8(0x5Au, mmu_read_byte(mmu, 0xD123u));

    mmu_write_byte(mmu, 0xFF68u, 0x80u);
    mmu_write_byte(mmu, 0xFF69u, 0x1Fu);
    TEST_ASSERT_EQUAL_HEX8(0u, mmu_read_byte(mmu, 0xFF69u));

    mmu_destroy(mmu);
    remove(phase11_rom);
}

void test_phase11_speed_switch(void)
{
    MMU *mmu = load_cgb_rom();
    CPU cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.pc = 1u;
    mmu_write_byte(mmu, 0xFF4Du, 1u);
    TEST_ASSERT_TRUE(cpu_execute_opcode(&cpu, mmu, 0x10u));
    TEST_ASSERT_TRUE(mmu->double_speed);
    TEST_ASSERT_EQUAL_HEX8(0u, mmu->speed_switch_pending);

    mmu_destroy(mmu);
    remove(phase11_rom);
}

void test_phase11_hdma_register(void)
{
    MMU *mmu = load_cgb_rom();

    mmu_write_byte(mmu, 0xFF51u, 0x00u);
    mmu_write_byte(mmu, 0xFF52u, 0x00u);
    mmu_write_byte(mmu, 0xFF53u, 0x80u);
    mmu_write_byte(mmu, 0xFF54u, 0x00u);
    mmu_write_byte(mmu, 0xFF55u, 0xFFu);
    TEST_ASSERT_TRUE(mmu->hdma_active);
    TEST_ASSERT_EQUAL_HEX8(128u, mmu->hdma_remaining);

    mmu_destroy(mmu);
    remove(phase11_rom);
}
