#include "unity.h"

void test_unity_assertions(void);
void test_cpu_executes_nop(void);
void test_cpu_step_fetches_nop(void);
void test_cpu_loads_bc_immediate(void);
void test_cpu_rejects_illegal_opcode(void);
void test_disassembler_decodes_immediate_and_relative_operands(void);
void test_disassembler_decodes_cb_and_stop_instructions(void);
void test_disassembler_decodes_illegal_opcode_and_wraps_address(void);
void test_disassembler_rejects_invalid_output_arguments(void);
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
void test_phase6_stat_mode_interrupt_edge(void);
void test_phase6_stat_hblank_interrupt(void);
void test_phase6_stat_lyc_interrupt(void);
void test_phase6_lcd_off_resets_ly_and_mode(void);
void test_phase6_bg_disabled_uses_color_zero_and_keeps_sprites(void);
void test_phase6_dmg_sprite_priority_uses_x_then_oam_order(void);
void test_phase6_obj_priority_uses_bg_color_index(void);
void test_phase6_window_line_counter_pauses_when_hidden(void);
void test_phase6_signed_tile_index_uses_9000_relative_base(void);
void test_phase9_breakpoints(void);
void test_phase9_debugger_state(void);
void test_phase10_save_and_load_state(void);
void test_phase10_reject_wrong_rom(void);
void test_phase11_cgb_header_and_vram(void);
void test_phase11_wram_and_palette(void);
void test_phase11_speed_switch(void);
void test_phase11_hdma_register(void);
void test_phase12_sgb_header(void);
void test_phase12_palette_packet(void);
void test_phase12_controller_packet(void);
void test_phase12_border_composition(void);

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
    RUN_TEST(test_disassembler_decodes_immediate_and_relative_operands);
    RUN_TEST(test_disassembler_decodes_cb_and_stop_instructions);
    RUN_TEST(test_disassembler_decodes_illegal_opcode_and_wraps_address);
    RUN_TEST(test_disassembler_rejects_invalid_output_arguments);
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
    RUN_TEST(test_phase6_stat_mode_interrupt_edge);
    RUN_TEST(test_phase6_stat_hblank_interrupt);
    RUN_TEST(test_phase6_stat_lyc_interrupt);
    RUN_TEST(test_phase6_lcd_off_resets_ly_and_mode);
    RUN_TEST(test_phase6_bg_disabled_uses_color_zero_and_keeps_sprites);
    RUN_TEST(test_phase6_dmg_sprite_priority_uses_x_then_oam_order);
    RUN_TEST(test_phase6_obj_priority_uses_bg_color_index);
    RUN_TEST(test_phase6_window_line_counter_pauses_when_hidden);
    RUN_TEST(test_phase6_signed_tile_index_uses_9000_relative_base);
    RUN_TEST(test_phase9_breakpoints);
    RUN_TEST(test_phase9_debugger_state);
    RUN_TEST(test_phase10_save_and_load_state);
    RUN_TEST(test_phase10_reject_wrong_rom);
    RUN_TEST(test_phase11_cgb_header_and_vram);
    RUN_TEST(test_phase11_wram_and_palette);
    RUN_TEST(test_phase11_speed_switch);
    RUN_TEST(test_phase11_hdma_register);
    RUN_TEST(test_phase12_sgb_header);
    RUN_TEST(test_phase12_palette_packet);
    RUN_TEST(test_phase12_controller_packet);
    RUN_TEST(test_phase12_border_composition);
    return UNITY_END();
}
