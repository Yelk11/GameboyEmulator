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

typedef struct CartridgeInfo {
    char *rom_name;
    bool loaded;
} CartridgeInfo;

typedef struct Timer {
    uint16_t div;
    uint32_t div_counter;
    uint32_t timer_counter;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
    bool overflow_pending;
} Timer;

typedef struct Dma {
    bool active;
    uint16_t source;
    uint16_t destination;
    uint32_t remaining;
} Dma;

typedef struct Serial {
    uint8_t sb;
    uint8_t sc;
    uint32_t transfer_counter;
} Serial;

typedef struct InterruptController {
    uint8_t ie;
    uint8_t if_;
    bool ime;
} InterruptController;

typedef struct Joypad {
    uint8_t action_buttons;
    uint8_t dpad_buttons;
    bool interrupt_pending;
} Joypad;

typedef struct EmulatorOptions {
    bool skip_boot_rom;
    bool paused;
} EmulatorOptions;

#define DEBUG_BREAKPOINT_MAX 16u

typedef struct Debugger {
    bool paused;
    bool step_instruction;
    bool step_frame;
    bool opcode_trace;
    bool memory_trace;
    uint16_t breakpoints[DEBUG_BREAKPOINT_MAX];
    uint8_t breakpoint_count;
} Debugger;

typedef struct GB {
    uint64_t total_t_cycles;
    uint64_t frame_number;
    CPU *cpu;
    MMU *mmu;
    PPU *ppu;
    Timer timer;
    InterruptController interrupt;
    Joypad joypad;
    Dma dma;
    Serial serial;
    CartridgeInfo cartridge;
    EmulatorOptions options;
    Debugger debugger;
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
void dma_init(Dma *dma);
void dma_update(Dma *dma, MMU *mmu, uint32_t t_cycles);
void serial_update(Serial *serial, uint32_t t_cycles);
uint32_t interrupt_step(GB *gb);
void joypad_press(Joypad *joypad, uint8_t button, bool pressed);
void debugger_init(Debugger *debugger);
bool debugger_add_breakpoint(Debugger *debugger, uint16_t address);
bool debugger_should_break(const Debugger *debugger, uint16_t pc);
void debugger_print_cpu(const GB *gb);
void debugger_print_ppu(const GB *gb);
void debugger_print_memory(const GB *gb, uint16_t address);
void debugger_trace_opcode(const GB *gb);

#endif