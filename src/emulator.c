#include <stdlib.h>
#include <string.h>

#include "gb.h"

void timer_init(Timer *timer)
{
    if (timer == NULL) {
        return;
    }

    timer->div = 0u;
    timer->tima = 0u;
    timer->tma = 0u;
    timer->tac = 0u;
}

void timer_update(Timer *timer, uint32_t t_cycles)
{
    if (timer != NULL) {
        timer->div = (uint16_t)((timer->div + t_cycles) & 0xFFFFu);
    }
}

void interrupt_init(InterruptController *interrupt)
{
    if (interrupt == NULL) {
        return;
    }

    interrupt->ie = 0u;
    interrupt->if_ = 0u;
    interrupt->ime = false;
}

void joypad_init(Joypad *joypad)
{
    if (joypad == NULL) {
        return;
    }

    joypad->action_buttons = 0x0Fu;
    joypad->dpad_buttons = 0x0Fu;
}

void gb_reset(GB *gb)
{
    if (gb == NULL) {
        return;
    }

    gb->total_t_cycles = 0u;
    gb->frame_number = 0u;
    gb->options.skip_boot_rom = true;
    gb->options.paused = false;

    if (gb->cpu != NULL) {
        cpu_init(gb->cpu);
    }
    if (gb->ppu != NULL) {
        ppu_init(gb->ppu);
    }

    timer_init(&gb->timer);
    interrupt_init(&gb->interrupt);
    joypad_init(&gb->joypad);
}

bool gb_init(GB *gb)
{
    if (gb == NULL) {
        return false;
    }

    memset(gb, 0, sizeof(*gb));
    gb->mmu = mmu_init();
    gb->cpu = calloc(1, sizeof(*gb->cpu));
    gb->ppu = calloc(1, sizeof(*gb->ppu));

    if (gb->mmu == NULL || gb->cpu == NULL || gb->ppu == NULL) {
        gb_destroy(gb);
        return false;
    }

    gb_reset(gb);
    gb->cartridge.loaded = false;
    gb->cartridge.rom_name = NULL;
    return true;
}

void gb_destroy(GB *gb)
{
    if (gb == NULL) {
        return;
    }

    mmu_destroy(gb->mmu);
    free(gb->cpu);
    free(gb->ppu);
    free(gb->cartridge.rom_name);

    gb->mmu = NULL;
    gb->cpu = NULL;
    gb->ppu = NULL;
    gb->cartridge.rom_name = NULL;
    gb->cartridge.loaded = false;
}

void gb_tick(GB *gb, uint32_t t_cycles)
{
    if (gb == NULL) {
        return;
    }

    gb->total_t_cycles += t_cycles;
    timer_update(&gb->timer, t_cycles);
    if (gb->ppu != NULL) {
        ppu_update(gb->ppu, t_cycles);
    }
}

void gb_run_frame(GB *gb)
{
    if (gb == NULL) {
        return;
    }

    gb_tick(gb, GB_T_CYCLES_PER_FRAME);
    gb->frame_number++;
}
