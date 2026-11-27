#include <stdio.h>
#include <string.h>

#include "gb.h"
#include "unity.h"

static const char *phase12_rom = "build/phase12.gb";

static MMU *load_sgb_rom(void)
{
    FILE *fp = fopen(phase12_rom, "wb");
    uint8_t rom[0x150];
    MMU *mmu;

    TEST_ASSERT_NOT_NULL(fp);
    memset(rom, 0, sizeof(rom));
    rom[0x146u] = 0x03u;
    rom[0x147u] = 0x00u;
    TEST_ASSERT_EQUAL_UINT32(sizeof(rom),
                             (uint32_t)fwrite(rom, 1, sizeof(rom), fp));
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));
    mmu = mmu_init();
    TEST_ASSERT_NOT_NULL(mmu);
    TEST_ASSERT_TRUE(mmu_load_rom(mmu, phase12_rom));
    return mmu;
}

void test_phase12_sgb_header(void)
{
    MMU *mmu = load_sgb_rom();

    TEST_ASSERT_TRUE(mmu_is_sgb(mmu));
    mmu_destroy(mmu);
    remove(phase12_rom);
}

void test_phase12_palette_packet(void)
{
    MMU *mmu = load_sgb_rom();
    uint8_t index;

    for (index = 0u; index < SGB_PACKET_SIZE; index++) {
        mmu_write_byte(mmu, 0xFF01u, index == 0u ? 0x00u : (uint8_t)(index * 2u));
    }
    TEST_ASSERT_EQUAL_UINT8(1u, mmu->sgb.packet_count);
    TEST_ASSERT_NOT_EQUAL_UINT32(0u, mmu->sgb.border[0]);

    mmu_destroy(mmu);
    remove(phase12_rom);
}

void test_phase12_controller_packet(void)
{
    MMU *mmu = load_sgb_rom();
    uint8_t index;

    for (index = 0u; index < SGB_PACKET_SIZE; index++) {
        mmu_write_byte(mmu, 0xFF01u,
                       index == 0u ? 0x88u : (uint8_t)(index == 1u ? 3u : 0u));
    }
    TEST_ASSERT_EQUAL_UINT8(3u, mmu->sgb.controller_count);

    mmu_destroy(mmu);
    remove(phase12_rom);
}

void test_phase12_border_composition(void)
{
    GB gb;
    MMU *mmu;

    mmu = load_sgb_rom();
    mmu_destroy(mmu);
    TEST_ASSERT_TRUE(gb_init(&gb));
    mmu_load_rom(gb.mmu, phase12_rom);
    if (gb.mmu->sgb.enabled) {
        gb.ppu->framebuffer[0] = 0xFFFFFFFFu;
        ppu_compose_sgb(gb.ppu);
        TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu,
                                 gb.ppu->sgb_framebuffer[40u * 256u + 48u]);
    }
    gb_destroy(&gb);
    remove(phase12_rom);
}
