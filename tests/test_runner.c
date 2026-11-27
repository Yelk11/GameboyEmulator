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
void test_cpu_add_and_flags(void);
void test_cpu_conditional_jr(void);
void test_cpu_call_and_return(void);
void test_cpu_cb_rotate(void);
void test_phase5_timer_overflow(void);
void test_phase5_joypad_interrupt(void);
void test_phase5_dma_transfer(void);
void test_phase5_interrupt_step(void);
void test_phase5_serial_transfer(void);
void test_phase6_scanline_timing(void);
void test_phase6_background_tile(void);
void test_phase6_vblank_interrupt(void);
void test_phase6_lyc_stat(void);
void test_phase6_window_and_sprite_registers(void);
void test_phase9_breakpoints(void);
void test_phase9_debugger_state(void);
void test_phase10_save_and_load_state(void);
void test_phase10_reject_wrong_rom(void);

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
    RUN_TEST(test_cpu_add_and_flags);
    RUN_TEST(test_cpu_conditional_jr);
    RUN_TEST(test_cpu_call_and_return);
    RUN_TEST(test_cpu_cb_rotate);
    RUN_TEST(test_phase5_timer_overflow);
    RUN_TEST(test_phase5_joypad_interrupt);
    RUN_TEST(test_phase5_dma_transfer);
    RUN_TEST(test_phase5_interrupt_step);
    RUN_TEST(test_phase5_serial_transfer);
    RUN_TEST(test_phase6_scanline_timing);
    RUN_TEST(test_phase6_background_tile);
    RUN_TEST(test_phase6_vblank_interrupt);
    RUN_TEST(test_phase6_lyc_stat);
    RUN_TEST(test_phase6_window_and_sprite_registers);
    RUN_TEST(test_phase9_breakpoints);
    RUN_TEST(test_phase9_debugger_state);
    RUN_TEST(test_phase10_save_and_load_state);
    RUN_TEST(test_phase10_reject_wrong_rom);
    return UNITY_END();
}
