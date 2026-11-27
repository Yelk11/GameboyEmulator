#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>

#include "mmu.h"

typedef struct CPU {
    // AF Register Pair
    union {
        struct {
            uint8_t f; // Flags (lower byte on little-endian)
            uint8_t a; // Accumulator (upper byte on little-endian)
        } bytes;
        uint16_t af;
    } AF_reg;

    // BC Register Pair
    union {
        struct {
            uint8_t c;
            uint8_t b;
        } bytes;
        uint16_t bc;
    } BC_reg;

    // DE Register Pair
    union {
        struct {
            uint8_t e;
            uint8_t d;
        } bytes;
        uint16_t de;
    } DE_reg;

    // HL Register Pair
    union {
        struct {
            uint8_t l;
            uint8_t h;
        } bytes;
        uint16_t hl;
    } HL_reg;

    uint16_t sp; // Stack Pointer
    uint16_t pc; // Program Counter
    uint32_t cycles;
    bool ime;
    bool halted;
    bool stopped;
    bool branch_taken;
    bool halt_bug;
} CPU;

typedef CPU reg;

typedef void (*cpu_opcode_handler)(CPU *cpu, MMU *mmu, uint8_t opcode);

void cpu_init(CPU *cpu);
bool cpu_execute_opcode(CPU *cpu, MMU *mmu, uint8_t opcode);
bool cpu_step(CPU *cpu, MMU *mmu);

#endif