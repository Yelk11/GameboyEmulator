#include <stdio.h>
#include <string.h>

#include "gb.h"

void debugger_init(Debugger *debugger)
{
    if (debugger == NULL) return;
    memset(debugger, 0, sizeof(*debugger));
}

bool debugger_add_breakpoint(Debugger *debugger, uint16_t address)
{
    if (debugger == NULL || debugger->breakpoint_count >= DEBUG_BREAKPOINT_MAX) {
        return false;
    }
    debugger->breakpoints[debugger->breakpoint_count++] = address;
    return true;
}

bool debugger_should_break(const Debugger *debugger, uint16_t pc)
{
    uint8_t index;
    if (debugger == NULL) return false;
    for (index = 0u; index < debugger->breakpoint_count; index++) {
        if (debugger->breakpoints[index] == pc) return true;
    }
    return false;
}

void debugger_print_cpu(const GB *gb)
{
    if (gb == NULL || gb->cpu == NULL) return;
    printf("CPU PC=%04X SP=%04X AF=%04X BC=%04X DE=%04X HL=%04X ",
           gb->cpu->pc, gb->cpu->sp,
           gb->cpu->AF_reg.af, gb->cpu->BC_reg.bc,
           gb->cpu->DE_reg.de, gb->cpu->HL_reg.hl);
    printf("IME=%u HALT=%u STOP=%u CYCLES=%u\n",
           gb->cpu->ime ? 1u : 0u,
           gb->cpu->halted ? 1u : 0u,
           gb->cpu->stopped ? 1u : 0u,
           gb->cpu->cycles);
}

void debugger_print_ppu(const GB *gb)
{
    if (gb == NULL || gb->ppu == NULL || gb->mmu == NULL) return;
    printf("PPU LY=%u LYC=%u MODE=%u DOT=%u ",
           gb->ppu->ly, gb->mmu->memory[0xFF45u],
           (unsigned)gb->ppu->mode, gb->ppu->line_dot);
    printf("LCDC=%02X STAT=%02X SCX=%u SCY=%u WX=%u WY=%u\n",
           gb->mmu->memory[0xFF40u], gb->mmu->memory[0xFF41u],
           gb->mmu->memory[0xFF43u], gb->mmu->memory[0xFF42u],
           gb->mmu->memory[0xFF4Bu], gb->mmu->memory[0xFF4Au]);
}

void debugger_print_memory(const GB *gb, uint16_t address)
{
    uint16_t index;
    if (gb == NULL || gb->mmu == NULL) return;
    printf("MEM %04X:", address & 0xFFF0u);
    for (index = 0u; index < 16u; index++) {
        uint16_t addr = (uint16_t)((address & 0xFFF0u) + index);
        printf(" %02X", mmu_read_byte(gb->mmu, addr));
    }
    printf("\n");
}

void debugger_trace_opcode(const GB *gb)
{
    if (gb == NULL || gb->cpu == NULL || gb->mmu == NULL) return;
    if (!gb->debugger.opcode_trace) return;
    printf("TRACE %04X A:%02X F:%02X BC:%04X DE:%04X HL:%04X SP:%04X\n",
           gb->cpu->pc,
           gb->cpu->AF_reg.bytes.a, gb->cpu->AF_reg.bytes.f,
           gb->cpu->BC_reg.bc, gb->cpu->DE_reg.de,
           gb->cpu->HL_reg.hl, gb->cpu->sp);
}
