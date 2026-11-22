#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>

#include "mmu.h"

typedef struct {
    // AF Register Pair
    union {
        struct {
            uint8_t f; // Flags (lower byte on little-endian)
            uint8_t a; // Accumulator (upper byte on little-endian)
        };
        uint16_t af;
    };

    // BC Register Pair
    union {
        struct {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc;
    };

    // DE Register Pair
    union {
        struct {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de;
    };

    // HL Register Pair
    union {
        struct {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl;
    };

    uint16_t sp; // Stack Pointer
    uint16_t pc; // Program Counter
} reg;

typedef void (*cpu_opcode_handler)(reg *cpu, MMU *mmu);

bool cpu_execute_opcode(reg *cpu, MMU *mmu, uint8_t opcode);

#endif