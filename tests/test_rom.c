#include "unity.h"

void test_load_rom(void)
{
    mmu_load_rom()
    TEST_ASSERT_EQUAL_INT(2, 1 + 1);
}
