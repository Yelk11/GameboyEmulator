#include <stdio.h>
#include <string.h>

#include "mmu.h"
#include "unity.h"

static const char *phase3_rom = "build/phase3.gb";

static MMU *create_test_rom(uint8_t type, uint8_t ram_code, size_t rom_size)
{
    FILE *fp = fopen(phase3_rom, "wb");
    MMU *mmu;
    size_t bank;
    uint8_t *rom;

    TEST_ASSERT_NOT_NULL(fp);
    rom = calloc(1, rom_size);
    TEST_ASSERT_NOT_NULL(rom);
    rom[0x147u] = type;
    rom[0x149u] = ram_code;
    for (bank = 0u; bank < rom_size / 0x4000u; bank++) {
        memset(rom + bank * 0x4000u, (int)(bank + 1u), 0x4000u);
    }
    rom[0x147u] = type;
    rom[0x149u] = ram_code;
    TEST_ASSERT_EQUAL_UINT32((uint32_t)rom_size,
                             (uint32_t)fwrite(rom, 1, rom_size, fp));
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));
    free(rom);

    mmu = mmu_init();
    TEST_ASSERT_NOT_NULL(mmu);
    TEST_ASSERT_TRUE(mmu_load_rom(mmu, phase3_rom));
    return mmu;
}

void test_mbc1_rom_and_ram_banking(void)
{
    MMU *mmu = create_test_rom(0x01u, 0x02u, 256u * 1024u);

    TEST_ASSERT_EQUAL_STRING("MBC1", mmu_cartridge_type_name(mmu));
    mmu_write_byte(mmu, 0x2000u, 0x02u);
    TEST_ASSERT_EQUAL_HEX8(3u, mmu_read_byte(mmu, 0x4000u));

    mmu_write_byte(mmu, 0x0000u, 0x0Au);
    mmu_write_byte(mmu, 0x4000u, 0x00u);
    mmu_write_byte(mmu, 0xA000u, 0x5Au);
    TEST_ASSERT_EQUAL_HEX8(0x5Au, mmu_read_byte(mmu, 0xA000u));

    mmu_destroy(mmu);
    remove(phase3_rom);
}

void test_mbc3_ram_enable_and_rtc(void)
{
    MMU *mmu = create_test_rom(0x10u, 0x03u, 128u * 1024u);

    TEST_ASSERT_EQUAL_STRING("MBC3", mmu_cartridge_type_name(mmu));
    mmu_write_byte(mmu, 0x0000u, 0x0Au);
    mmu_write_byte(mmu, 0x4000u, 0x08u);
    mmu_write_byte(mmu, 0xA000u, 37u);
    TEST_ASSERT_EQUAL_HEX8(37u, mmu_read_byte(mmu, 0xA000u));

    mmu_destroy(mmu);
    remove(phase3_rom);
}

void test_mbc5_rom_and_ram_banking(void)
{
    MMU *mmu = create_test_rom(0x1Bu, 0x02u, 512u * 1024u);

    TEST_ASSERT_EQUAL_STRING("MBC5", mmu_cartridge_type_name(mmu));
    mmu_write_byte(mmu, 0x2000u, 0xFFu);
    mmu_write_byte(mmu, 0x3000u, 0x01u);
    TEST_ASSERT_EQUAL_HEX8(0x20u, mmu_read_byte(mmu, 0x4000u));

    mmu_destroy(mmu);
    remove(phase3_rom);
}

void test_echo_ram_mirroring(void)
{
    MMU *mmu = mmu_init();

    TEST_ASSERT_NOT_NULL(mmu);
    mmu_write_byte(mmu, 0xC123u, 0xA5u);
    TEST_ASSERT_EQUAL_HEX8(0xA5u, mmu_read_byte(mmu, 0xE123u));
    mmu_write_byte(mmu, 0xE456u, 0x5Au);
    TEST_ASSERT_EQUAL_HEX8(0x5Au, mmu_read_byte(mmu, 0xC456u));

    mmu_destroy(mmu);
}
