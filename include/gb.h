#ifndef GB_H
#define GB_H

#include <stdbool.h>
#include <stdint.h>

#include "mmu.h"
#include "cpu.h"
#include "ppu.h"

#define GB_CLOCK_HZ 4194304u
#define GB_T_CYCLES_PER_SCANLINE 456u
#define GB_SCANLINE_COUNT 154u
#define GB_T_CYCLES_PER_FRAME 70224u
#define GB_FRAMEBUFFER_WIDTH 160u
#define GB_FRAMEBUFFER_HEIGHT 144u

typedef struct Cartridge {
    char *rom_name;
    bool loaded;
} Cartridge;

typedef struct Timer {
    uint16_t div;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
} Timer;

typedef struct InterruptController {
    uint8_t ie;
    uint8_t if_;
    bool ime;
} InterruptController;

typedef struct Joypad {
    uint8_t action_buttons;
    uint8_t dpad_buttons;
} Joypad;

typedef struct EmulatorOptions {
    bool skip_boot_rom;
    bool paused;
} EmulatorOptions;

typedef struct GB {
    uint64_t total_t_cycles;
    uint64_t frame_number;
    CPU *cpu;
    MMU *mmu;
    PPU *ppu;
    Timer timer;
    InterruptController interrupt;
    Joypad joypad;
    Cartridge cartridge;
    EmulatorOptions options;
} GB;

bool gb_init(GB *gb);
void gb_destroy(GB *gb);
void gb_reset(GB *gb);
void gb_tick(GB *gb, uint32_t t_cycles);
void gb_run_frame(GB *gb);

void timer_init(Timer *timer);
void timer_update(Timer *timer, uint32_t t_cycles);
void interrupt_init(InterruptController *interrupt);
void joypad_init(Joypad *joypad);

#endif