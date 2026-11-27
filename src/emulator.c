#include <stdlib.h>
#include <string.h>

#include "gb.h"

void timer_init(Timer *timer)
{
    if (timer == NULL) {
        return;
    }

    timer->div = 0u;
    timer->div_counter = 0u;
    timer->timer_counter = 0u;
    timer->tima = 0u;
    timer->tma = 0u;
    timer->tac = 0u;
    timer->overflow_pending = false;
}

void timer_update(Timer *timer, uint32_t t_cycles)
{
    uint32_t threshold;
    if (timer == NULL) return;

    timer->div_counter += t_cycles;
    timer->div = (uint16_t)((timer->div + t_cycles) & 0xFFFFu);

    if ((timer->tac & 0x04u) == 0u) return;
    switch (timer->tac & 0x03u) {
    case 0: threshold = 1024u; break;
    case 1: threshold = 16u; break;
    case 2: threshold = 64u; break;
    default: threshold = 256u; break;
    }
    timer->timer_counter += t_cycles;
    while (timer->timer_counter >= threshold) {
        timer->timer_counter -= threshold;
        if (timer->tima == 0xFFu) {
            timer->tima = timer->tma;
            timer->overflow_pending = true;
        } else {
            timer->tima++;
        }
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
    joypad->interrupt_pending = false;
}

void dma_init(Dma *dma)
{
    if (dma == NULL) return;
    dma->active = false;
    dma->source = 0u;
    dma->destination = 0xFE00u;
    dma->remaining = 0u;
}

void dma_update(Dma *dma, MMU *mmu, uint32_t t_cycles)
{
    uint32_t cycles;
    if (dma == NULL || mmu == NULL) return;
    if (mmu->dma_active) {
        dma->active = true;
        dma->source = (uint16_t)((uint16_t)mmu->dma_source << 8);
        if (dma->remaining == 0u) dma->remaining = mmu->dma_remaining;
    }
    if (!dma->active) return;

    cycles = t_cycles;
    while (cycles >= 4u && dma->remaining > 0u) {
        uint16_t offset = (uint16_t)(160u - dma->remaining);
        mmu->memory[0xFE00u + offset] =
            mmu_read_byte(mmu, (uint16_t)(dma->source + offset));
        dma->remaining--;
        cycles -= 4u;
    }
    if (dma->remaining == 0u) {
        dma->active = false;
        mmu->dma_active = false;
    }
}

void serial_update(Serial *serial, uint32_t t_cycles)
{
    if (serial == NULL) return;
    serial->transfer_counter += t_cycles;
    if ((serial->sc & 0x80u) != 0u && serial->transfer_counter >= 512u) {
        serial->transfer_counter = 0u;
        serial->sc &= 0x7Fu;
    }
}

void joypad_press(Joypad *joypad, uint8_t button, bool pressed)
{
    uint8_t mask;
    if (joypad == NULL) return;
    if (pressed) joypad->interrupt_pending = true;
    if (button >= 4u) {
        mask = (uint8_t)(1u << (button - 4u));
        if (pressed) joypad->dpad_buttons &= (uint8_t)~mask;
        else joypad->dpad_buttons |= mask;
    } else {
        mask = (uint8_t)(1u << button);
        if (pressed) joypad->action_buttons &= (uint8_t)~mask;
        else joypad->action_buttons |= mask;
    }
}

uint32_t interrupt_step(GB *gb)
{
    uint8_t pending;
    uint8_t source;
    uint16_t vector;
    if (gb == NULL || gb->cpu == NULL || gb->mmu == NULL) return 0u;
    if (!gb->cpu->ime) return 0u;

    gb->interrupt.ie = gb->mmu->memory[0xFFFFu];
    gb->interrupt.if_ = gb->mmu->memory[0xFF0Fu];
    pending = (uint8_t)(gb->interrupt.ie & gb->interrupt.if_);
    if (pending != 0u && gb->cpu->halted) {
        gb->cpu->halted = false;
    }
    if (pending == 0u) return 0u;

    if ((pending & 0x01u) != 0u) { source = 0x01u; vector = 0x0040u; }
    else if ((pending & 0x02u) != 0u) { source = 0x02u; vector = 0x0048u; }
    else if ((pending & 0x04u) != 0u) { source = 0x04u; vector = 0x0050u; }
    else if ((pending & 0x08u) != 0u) { source = 0x08u; vector = 0x0058u; }
    else if ((pending & 0x10u) != 0u) { source = 0x10u; vector = 0x0060u; }
    else return 0u;

    gb->cpu->ime = false;
    gb->interrupt.ime = false;
    gb->cpu->sp = (uint16_t)(gb->cpu->sp - 2u);
    mmu_write_byte(gb->mmu, gb->cpu->sp, (uint8_t)gb->cpu->pc);
    mmu_write_byte(gb->mmu, (uint16_t)(gb->cpu->sp + 1u),
                   (uint8_t)(gb->cpu->pc >> 8));
    gb->cpu->pc = vector;
    gb->mmu->memory[0xFF0Fu] &= (uint8_t)~source;
    return 20u;
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
        ppu_connect(gb->ppu, gb->mmu);
    }

    timer_init(&gb->timer);
    interrupt_init(&gb->interrupt);
    joypad_init(&gb->joypad);
    dma_init(&gb->dma);
    debugger_init(&gb->debugger);
    gb->serial.sb = 0u;
    gb->serial.sc = 0u;
    gb->serial.transfer_counter = 0u;
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

    if (gb->mmu != NULL) {
        mmu_save_battery(gb->mmu);
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
    gb->timer.tima = gb->mmu->memory[0xFF05u];
    gb->timer.tma = gb->mmu->memory[0xFF06u];
    gb->timer.tac = gb->mmu->memory[0xFF07u];
    gb->mmu->action_buttons = gb->joypad.action_buttons;
    gb->mmu->dpad_buttons = gb->joypad.dpad_buttons;
    timer_update(&gb->timer, t_cycles);
    if (gb->timer.overflow_pending) {
        gb->mmu->memory[0xFF0Fu] |= 0x04u;
        gb->timer.overflow_pending = false;
    }
    if (gb->joypad.interrupt_pending) {
        gb->mmu->memory[0xFF0Fu] |= 0x10u;
        gb->joypad.interrupt_pending = false;
    }
    gb->mmu->memory[0xFF05u] = gb->timer.tima;
    gb->mmu->memory[0xFF06u] = gb->timer.tma;
    gb->mmu->memory[0xFF07u] = gb->timer.tac;
    gb->mmu->memory[0xFF04u] = (uint8_t)(gb->timer.div >> 8);
    if (gb->mmu->cgb_mode && gb->mmu->hdma_active) {
        uint16_t source = (uint16_t)(((uint16_t)gb->mmu->hdma_source[0] << 8) |
                                     gb->mmu->hdma_source[1]);
        uint16_t destination = (uint16_t)(0x8000u +
            (((uint16_t)gb->mmu->hdma_destination[0] << 8 |
              gb->mmu->hdma_destination[1]) & 0x1FF0u));
        if (t_cycles >= 456u && gb->ppu->mode == PPU_MODE_HBLANK) {
            uint16_t index;
            for (index = 0u; index < 16u; index++) {
                mmu_write_byte(gb->mmu,
                               (uint16_t)(destination + index),
                               mmu_read_byte(gb->mmu,
                                             (uint16_t)(source + index)));
            }
            gb->mmu->hdma_source[0] = (uint8_t)(source >> 8);
            gb->mmu->hdma_source[1] = (uint8_t)(source & 0xF0u);
            gb->mmu->hdma_destination[0] = (uint8_t)((destination >> 8) & 0x1Fu);
            gb->mmu->hdma_destination[1] = (uint8_t)(destination & 0xF0u);
            if (gb->mmu->hdma_remaining > 0u)
                gb->mmu->hdma_remaining--;
            gb->mmu->memory[0xFF55u] =
                gb->mmu->hdma_remaining == 0u ?
                0xFFu : (uint8_t)(gb->mmu->hdma_remaining - 1u);
            if (gb->mmu->hdma_remaining == 0u)
                gb->mmu->hdma_active = false;
        }
    }
    dma_update(&gb->dma, gb->mmu, t_cycles);
    gb->serial.sb = gb->mmu->memory[0xFF01u];
    gb->serial.sc = gb->mmu->memory[0xFF02u];
    serial_update(&gb->serial, t_cycles);
    gb->mmu->memory[0xFF01u] = gb->serial.sb;
    gb->mmu->memory[0xFF02u] = gb->serial.sc;
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
