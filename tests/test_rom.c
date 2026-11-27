#include "mmu.h"
#include "unity.h"

#include <stdio.h>
#include <string.h>

static const char *test_rom_path = "build/test_rom.bin";

void test_load_rom(void)
{
    FILE *fp;
    const uint8_t rom[] = {0x00, 0x01, 0xFE, 0xFF};
    MMU *mmu;

    fp = fopen(test_rom_path, "wb");
    TEST_ASSERT_NOT_NULL(fp);
    TEST_ASSERT_EQUAL_UINT32(4, fwrite(rom, 1, sizeof(rom), fp));
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));

    mmu = mmu_init();
    TEST_ASSERT_NOT_NULL(mmu);
    TEST_ASSERT_TRUE(mmu_load_rom(mmu, test_rom_path));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(rom, mmu->memory, sizeof(rom));

    mmu_destroy(mmu);
    remove(test_rom_path);
}

void test_load_rom_missing_file(void)
{
    MMU *mmu = mmu_init();

    TEST_ASSERT_NOT_NULL(mmu);
    TEST_ASSERT_FALSE(mmu_load_rom(mmu, "build/does-not-exist.gb"));
    mmu_destroy(mmu);
}
