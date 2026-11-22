#include "cpu.h"

#include <stdint.h>
#include <stdio.h>

// Forward declaration of your CPU state struct
typedef struct CPU CPU;

// Function pointer type for opcode execution
typedef void (*OpcodeHandler)(CPU *cpu);

typedef struct {
    const char *mnemonic;   // For debugging/disassembly (e.g., "LD BC, d16")
    uint8_t length;         // Instruction size in bytes (1 to 3)
    uint8_t cycles;         // Base T-cycles (or M-cycles)
    OpcodeHandler execute;  // Pointer to the handler function
} Instruction;

// Example CPU struct
struct CPU {
    uint16_t pc;
    uint16_t sp;
    uint8_t a, b, c, d, e, f, h, l;
    uint8_t memory[0x10000];
    uint32_t cycles;
    // Add interrupt flags, halt state, etc.
};

// --- Example Opcode Handlers ---

void op_unimplemented(CPU *cpu) {
    uint8_t opcode = cpu->memory[cpu->pc - 1];
    printf("Unimplemented opcode: 0x%02X at PC: 0x%04X\n", opcode, cpu->pc - 1);
}

void op_00_nop(CPU *cpu) {
    // Do nothing
}

void op_01_ld_bc_d16(CPU *cpu) {
    uint8_t low  = cpu->memory[cpu->pc++];
    uint8_t high = cpu->memory[cpu->pc++];
    cpu->c = low;
    cpu->b = high;
}

void op_cb_prefix(CPU *cpu); // Forward declaration for 0xCB handler

// --- Primary Opcode Table (256 entries) ---

const Instruction opcodes[256] = {
/*   OP code    name            Size    cycles  function*/    
    [0x00] = { "NOP",           1,      2,      op_00_nop },
    [0x01] = { "LD BC, d16",    3,      3,      op_01_ld_bc_d16 },
    [0x02] = { "LD (BC), A",    1,      2,      op_unimplemented},
    [0x03] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0x02] = { "LD BC, d16",    3,      3,      op_unimplemented},
    [0xCB] = { "PREFIX CB",     1,      4,  op_cb_prefix },
};

// --- CB-Prefixed Opcode Table (256 entries) ---

void op_cb_00_rlc_b(CPU *cpu) {
    // Rotate B left logic
}

const Instruction cb_opcodes[256] = {
    [0x00] = { "RLC B", 2, 8, op_cb_00_rlc_b },
    // ... populate remaining 255 CB opcodes ...
};

// --- Execution Loop ---

void op_cb_prefix(CPU *cpu) {
    uint8_t cb_op = cpu->memory[cpu->pc++];
    const Instruction *inst = &cb_opcodes[cb_op];
    
    if (inst->execute) {
        inst->execute(cpu);
        cpu->cycles += inst->cycles;
    } else {
        op_unimplemented(cpu);
    }
}

void cpu_step(CPU *cpu) {
    uint8_t opcode = cpu->memory[cpu->pc++];
    const Instruction *inst = &opcodes[opcode];

    if (inst->execute) {
        inst->execute(cpu);
        cpu->cycles += inst->cycles;
    } else {
        op_unimplemented(cpu);
    }
}



