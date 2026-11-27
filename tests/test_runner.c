#include "unity.h"

void test_unity_assertions(void);
void test_cpu_executes_nop(void);
void test_cpu_step_fetches_nop(void);
void test_cpu_loads_bc_immediate(void);
void test_cpu_rejects_illegal_opcode(void);
void test_load_rom(void);
void test_load_rom_missing_file(void);
void test_gb_initializes_subsystems(void);
void test_gb_reset_sets_post_boot_state(void);
void test_gb_tick_advances_scanline(void);
void test_gb_run_frame_advances_exact_cycles(void);
void test_mbc1_rom_and_ram_banking(void);
void test_mbc3_ram_enable_and_rtc(void);
void test_mbc5_rom_and_ram_banking(void);
void test_echo_ram_mirroring(void);

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unity_assertions);
    RUN_TEST(test_cpu_executes_nop);
    RUN_TEST(test_cpu_step_fetches_nop);
    RUN_TEST(test_cpu_loads_bc_immediate);
    RUN_TEST(test_cpu_rejects_illegal_opcode);
    RUN_TEST(test_load_rom);
    RUN_TEST(test_load_rom_missing_file);
    RUN_TEST(test_gb_initializes_subsystems);
    RUN_TEST(test_gb_reset_sets_post_boot_state);
    RUN_TEST(test_gb_tick_advances_scanline);
    RUN_TEST(test_gb_run_frame_advances_exact_cycles);
    RUN_TEST(test_mbc1_rom_and_ram_banking);
    RUN_TEST(test_mbc3_ram_enable_and_rtc);
    RUN_TEST(test_mbc5_rom_and_ram_banking);
    RUN_TEST(test_echo_ram_mirroring);
    return UNITY_END();
}
