#include "cpu.h"

#include <stdio.h>

void cpu_init(CPU *cpu)
{
    if (cpu == NULL) {
        return;
    }

    cpu->AF_reg.af = 0x01B0u;
    cpu->BC_reg.bc = 0x0013u;
    cpu->DE_reg.de = 0x00D8u;
    cpu->HL_reg.hl = 0x014Du;
    cpu->sp = 0xFFFEu;
    cpu->pc = 0x0100u;
    cpu->cycles = 0u;
    cpu->ime = false;
    cpu->halted = false;
    cpu->stopped = false;
    cpu->branch_taken = true;
    cpu->halt_bug = false;
    cpu->halt_bug = false;
}

typedef struct {
    const char *mnemonic;
    uint8_t length;
    uint8_t cycles;
    uint8_t alternate_cycles;
    cpu_opcode_handler execute;
} Instruction;

static void op_unimplemented(CPU *cpu, MMU *mmu, uint8_t opcode)
{
    (void)mmu;
    printf("Unimplemented opcode: 0x%02X at PC: 0x%04X\n",
           opcode, (uint16_t)(cpu->pc - 1));
}

static void op_00_nop(CPU *cpu, MMU *mmu, uint8_t opcode)
{
    (void)cpu;
    (void)mmu;
    (void)opcode;
}

static void op_01_ld_bc_d16(CPU *cpu, MMU *mmu, uint8_t opcode)
{
    (void)opcode;
    cpu->BC_reg.bytes.c = mmu_read_byte(mmu, cpu->pc++);
    cpu->BC_reg.bytes.b = mmu_read_byte(mmu, cpu->pc++);
}

static void op_cb_prefix(CPU *cpu, MMU *mmu, uint8_t opcode);



/*
 * Opcode metadata derived from lmmendes/game-boy-opcodes (MIT);
 * see licenses/game-boy-opcodes.MIT.
 * Cycles are T-cycles; alternate_cycles is used for the not-taken path.
 */
static const Instruction opcodes[256] = {
    [0x00] = { "NOP", 1, 4, 0, op_00_nop },
    [0x01] = { "LD BC, d16", 3, 12, 0, op_01_ld_bc_d16 },
    [0x02] = { "LD (BC), A", 1, 8, 0, op_unimplemented },
    [0x03] = { "INC BC", 1, 8, 0, op_unimplemented },
    [0x04] = { "INC B", 1, 4, 0, op_unimplemented },
    [0x05] = { "DEC B", 1, 4, 0, op_unimplemented },
    [0x06] = { "LD B, d8", 2, 8, 0, op_unimplemented },
    [0x07] = { "RLCA", 1, 4, 0, op_unimplemented },
    [0x08] = { "LD (a16), SP", 3, 20, 0, op_unimplemented },
    [0x09] = { "ADD HL, BC", 1, 8, 0, op_unimplemented },
    [0x0a] = { "LD A, (BC)", 1, 8, 0, op_unimplemented },
    [0x0b] = { "DEC BC", 1, 8, 0, op_unimplemented },
    [0x0c] = { "INC C", 1, 4, 0, op_unimplemented },
    [0x0d] = { "DEC C", 1, 4, 0, op_unimplemented },
    [0x0e] = { "LD C, d8", 2, 8, 0, op_unimplemented },
    [0x0f] = { "RRCA", 1, 4, 0, op_unimplemented },
    [0x10] = { "STOP 0", 1, 4, 0, op_unimplemented },
    [0x11] = { "LD DE, d16", 3, 12, 0, op_unimplemented },
    [0x12] = { "LD (DE), A", 1, 8, 0, op_unimplemented },
    [0x13] = { "INC DE", 1, 8, 0, op_unimplemented },
    [0x14] = { "INC D", 1, 4, 0, op_unimplemented },
    [0x15] = { "DEC D", 1, 4, 0, op_unimplemented },
    [0x16] = { "LD D, d8", 2, 8, 0, op_unimplemented },
    [0x17] = { "RLA", 1, 4, 0, op_unimplemented },
    [0x18] = { "JR r8", 2, 12, 0, op_unimplemented },
    [0x19] = { "ADD HL, DE", 1, 8, 0, op_unimplemented },
    [0x1a] = { "LD A, (DE)", 1, 8, 0, op_unimplemented },
    [0x1b] = { "DEC DE", 1, 8, 0, op_unimplemented },
    [0x1c] = { "INC E", 1, 4, 0, op_unimplemented },
    [0x1d] = { "DEC E", 1, 4, 0, op_unimplemented },
    [0x1e] = { "LD E, d8", 2, 8, 0, op_unimplemented },
    [0x1f] = { "RRA", 1, 4, 0, op_unimplemented },
    [0x20] = { "JR NZ, r8", 2, 12, 8, op_unimplemented },
    [0x21] = { "LD HL, d16", 3, 12, 0, op_unimplemented },
    [0x22] = { "LD (HL+), A", 1, 8, 0, op_unimplemented },
    [0x23] = { "INC HL", 1, 8, 0, op_unimplemented },
    [0x24] = { "INC H", 1, 4, 0, op_unimplemented },
    [0x25] = { "DEC H", 1, 4, 0, op_unimplemented },
    [0x26] = { "LD H, d8", 2, 8, 0, op_unimplemented },
    [0x27] = { "DAA", 1, 4, 0, op_unimplemented },
    [0x28] = { "JR Z, r8", 2, 12, 8, op_unimplemented },
    [0x29] = { "ADD HL, HL", 1, 8, 0, op_unimplemented },
    [0x2a] = { "LD A, (HL+)", 1, 8, 0, op_unimplemented },
    [0x2b] = { "DEC HL", 1, 8, 0, op_unimplemented },
    [0x2c] = { "INC L", 1, 4, 0, op_unimplemented },
    [0x2d] = { "DEC L", 1, 4, 0, op_unimplemented },
    [0x2e] = { "LD L, d8", 2, 8, 0, op_unimplemented },
    [0x2f] = { "CPL", 1, 4, 0, op_unimplemented },
    [0x30] = { "JR NC, r8", 2, 12, 8, op_unimplemented },
    [0x31] = { "LD SP, d16", 3, 12, 0, op_unimplemented },
    [0x32] = { "LD (HL-), A", 1, 8, 0, op_unimplemented },
    [0x33] = { "INC SP", 1, 8, 0, op_unimplemented },
    [0x34] = { "INC (HL)", 1, 12, 0, op_unimplemented },
    [0x35] = { "DEC (HL)", 1, 12, 0, op_unimplemented },
    [0x36] = { "LD (HL), d8", 2, 12, 0, op_unimplemented },
    [0x37] = { "SCF", 1, 4, 0, op_unimplemented },
    [0x38] = { "JR C, r8", 2, 12, 8, op_unimplemented },
    [0x39] = { "ADD HL, SP", 1, 8, 0, op_unimplemented },
    [0x3a] = { "LD A, (HL-)", 1, 8, 0, op_unimplemented },
    [0x3b] = { "DEC SP", 1, 8, 0, op_unimplemented },
    [0x3c] = { "INC A", 1, 4, 0, op_unimplemented },
    [0x3d] = { "DEC A", 1, 4, 0, op_unimplemented },
    [0x3e] = { "LD A, d8", 2, 8, 0, op_unimplemented },
    [0x3f] = { "CCF", 1, 4, 0, op_unimplemented },
    [0x40] = { "LD B, B", 1, 4, 0, op_unimplemented },
    [0x41] = { "LD B, C", 1, 4, 0, op_unimplemented },
    [0x42] = { "LD B, D", 1, 4, 0, op_unimplemented },
    [0x43] = { "LD B, E", 1, 4, 0, op_unimplemented },
    [0x44] = { "LD B, H", 1, 4, 0, op_unimplemented },
    [0x45] = { "LD B, L", 1, 4, 0, op_unimplemented },
    [0x46] = { "LD B, (HL)", 1, 8, 0, op_unimplemented },
    [0x47] = { "LD B, A", 1, 4, 0, op_unimplemented },
    [0x48] = { "LD C, B", 1, 4, 0, op_unimplemented },
    [0x49] = { "LD C, C", 1, 4, 0, op_unimplemented },
    [0x4a] = { "LD C, D", 1, 4, 0, op_unimplemented },
    [0x4b] = { "LD C, E", 1, 4, 0, op_unimplemented },
    [0x4c] = { "LD C, H", 1, 4, 0, op_unimplemented },
    [0x4d] = { "LD C, L", 1, 4, 0, op_unimplemented },
    [0x4e] = { "LD C, (HL)", 1, 8, 0, op_unimplemented },
    [0x4f] = { "LD C, A", 1, 4, 0, op_unimplemented },
    [0x50] = { "LD D, B", 1, 4, 0, op_unimplemented },
    [0x51] = { "LD D, C", 1, 4, 0, op_unimplemented },
    [0x52] = { "LD D, D", 1, 4, 0, op_unimplemented },
    [0x53] = { "LD D, E", 1, 4, 0, op_unimplemented },
    [0x54] = { "LD D, H", 1, 4, 0, op_unimplemented },
    [0x55] = { "LD D, L", 1, 4, 0, op_unimplemented },
    [0x56] = { "LD D, (HL)", 1, 8, 0, op_unimplemented },
    [0x57] = { "LD D, A", 1, 4, 0, op_unimplemented },
    [0x58] = { "LD E, B", 1, 4, 0, op_unimplemented },
    [0x59] = { "LD E, C", 1, 4, 0, op_unimplemented },
    [0x5a] = { "LD E, D", 1, 4, 0, op_unimplemented },
    [0x5b] = { "LD E, E", 1, 4, 0, op_unimplemented },
    [0x5c] = { "LD E, H", 1, 4, 0, op_unimplemented },
    [0x5d] = { "LD E, L", 1, 4, 0, op_unimplemented },
    [0x5e] = { "LD E, (HL)", 1, 8, 0, op_unimplemented },
    [0x5f] = { "LD E, A", 1, 4, 0, op_unimplemented },
    [0x60] = { "LD H, B", 1, 4, 0, op_unimplemented },
    [0x61] = { "LD H, C", 1, 4, 0, op_unimplemented },
    [0x62] = { "LD H, D", 1, 4, 0, op_unimplemented },
    [0x63] = { "LD H, E", 1, 4, 0, op_unimplemented },
    [0x64] = { "LD H, H", 1, 4, 0, op_unimplemented },
    [0x65] = { "LD H, L", 1, 4, 0, op_unimplemented },
    [0x66] = { "LD H, (HL)", 1, 8, 0, op_unimplemented },
    [0x67] = { "LD H, A", 1, 4, 0, op_unimplemented },
    [0x68] = { "LD L, B", 1, 4, 0, op_unimplemented },
    [0x69] = { "LD L, C", 1, 4, 0, op_unimplemented },
    [0x6a] = { "LD L, D", 1, 4, 0, op_unimplemented },
    [0x6b] = { "LD L, E", 1, 4, 0, op_unimplemented },
    [0x6c] = { "LD L, H", 1, 4, 0, op_unimplemented },
    [0x6d] = { "LD L, L", 1, 4, 0, op_unimplemented },
    [0x6e] = { "LD L, (HL)", 1, 8, 0, op_unimplemented },
    [0x6f] = { "LD L, A", 1, 4, 0, op_unimplemented },
    [0x70] = { "LD (HL), B", 1, 8, 0, op_unimplemented },
    [0x71] = { "LD (HL), C", 1, 8, 0, op_unimplemented },
    [0x72] = { "LD (HL), D", 1, 8, 0, op_unimplemented },
    [0x73] = { "LD (HL), E", 1, 8, 0, op_unimplemented },
    [0x74] = { "LD (HL), H", 1, 8, 0, op_unimplemented },
    [0x75] = { "LD (HL), L", 1, 8, 0, op_unimplemented },
    [0x76] = { "HALT", 1, 4, 0, op_unimplemented },
    [0x77] = { "LD (HL), A", 1, 8, 0, op_unimplemented },
    [0x78] = { "LD A, B", 1, 4, 0, op_unimplemented },
    [0x79] = { "LD A, C", 1, 4, 0, op_unimplemented },
    [0x7a] = { "LD A, D", 1, 4, 0, op_unimplemented },
    [0x7b] = { "LD A, E", 1, 4, 0, op_unimplemented },
    [0x7c] = { "LD A, H", 1, 4, 0, op_unimplemented },
    [0x7d] = { "LD A, L", 1, 4, 0, op_unimplemented },
    [0x7e] = { "LD A, (HL)", 1, 8, 0, op_unimplemented },
    [0x7f] = { "LD A, A", 1, 4, 0, op_unimplemented },
    [0x80] = { "ADD A, B", 1, 4, 0, op_unimplemented },
    [0x81] = { "ADD A, C", 1, 4, 0, op_unimplemented },
    [0x82] = { "ADD A, D", 1, 4, 0, op_unimplemented },
    [0x83] = { "ADD A, E", 1, 4, 0, op_unimplemented },
    [0x84] = { "ADD A, H", 1, 4, 0, op_unimplemented },
    [0x85] = { "ADD A, L", 1, 4, 0, op_unimplemented },
    [0x86] = { "ADD A, (HL)", 1, 8, 0, op_unimplemented },
    [0x87] = { "ADD A, A", 1, 4, 0, op_unimplemented },
    [0x88] = { "ADC A, B", 1, 4, 0, op_unimplemented },
    [0x89] = { "ADC A, C", 1, 4, 0, op_unimplemented },
    [0x8a] = { "ADC A, D", 1, 4, 0, op_unimplemented },
    [0x8b] = { "ADC A, E", 1, 4, 0, op_unimplemented },
    [0x8c] = { "ADC A, H", 1, 4, 0, op_unimplemented },
    [0x8d] = { "ADC A, L", 1, 4, 0, op_unimplemented },
    [0x8e] = { "ADC A, (HL)", 1, 8, 0, op_unimplemented },
    [0x8f] = { "ADC A, A", 1, 4, 0, op_unimplemented },
    [0x90] = { "SUB B", 1, 4, 0, op_unimplemented },
    [0x91] = { "SUB C", 1, 4, 0, op_unimplemented },
    [0x92] = { "SUB D", 1, 4, 0, op_unimplemented },
    [0x93] = { "SUB E", 1, 4, 0, op_unimplemented },
    [0x94] = { "SUB H", 1, 4, 0, op_unimplemented },
    [0x95] = { "SUB L", 1, 4, 0, op_unimplemented },
    [0x96] = { "SUB (HL)", 1, 8, 0, op_unimplemented },
    [0x97] = { "SUB A", 1, 4, 0, op_unimplemented },
    [0x98] = { "SBC A, B", 1, 4, 0, op_unimplemented },
    [0x99] = { "SBC A, C", 1, 4, 0, op_unimplemented },
    [0x9a] = { "SBC A, D", 1, 4, 0, op_unimplemented },
    [0x9b] = { "SBC A, E", 1, 4, 0, op_unimplemented },
    [0x9c] = { "SBC A, H", 1, 4, 0, op_unimplemented },
    [0x9d] = { "SBC A, L", 1, 4, 0, op_unimplemented },
    [0x9e] = { "SBC A, (HL)", 1, 8, 0, op_unimplemented },
    [0x9f] = { "SBC A, A", 1, 4, 0, op_unimplemented },
    [0xa0] = { "AND B", 1, 4, 0, op_unimplemented },
    [0xa1] = { "AND C", 1, 4, 0, op_unimplemented },
    [0xa2] = { "AND D", 1, 4, 0, op_unimplemented },
    [0xa3] = { "AND E", 1, 4, 0, op_unimplemented },
    [0xa4] = { "AND H", 1, 4, 0, op_unimplemented },
    [0xa5] = { "AND L", 1, 4, 0, op_unimplemented },
    [0xa6] = { "AND (HL)", 1, 8, 0, op_unimplemented },
    [0xa7] = { "AND A", 1, 4, 0, op_unimplemented },
    [0xa8] = { "XOR B", 1, 4, 0, op_unimplemented },
    [0xa9] = { "XOR C", 1, 4, 0, op_unimplemented },
    [0xaa] = { "XOR D", 1, 4, 0, op_unimplemented },
    [0xab] = { "XOR E", 1, 4, 0, op_unimplemented },
    [0xac] = { "XOR H", 1, 4, 0, op_unimplemented },
    [0xad] = { "XOR L", 1, 4, 0, op_unimplemented },
    [0xae] = { "XOR (HL)", 1, 8, 0, op_unimplemented },
    [0xaf] = { "XOR A", 1, 4, 0, op_unimplemented },
    [0xb0] = { "OR B", 1, 4, 0, op_unimplemented },
    [0xb1] = { "OR C", 1, 4, 0, op_unimplemented },
    [0xb2] = { "OR D", 1, 4, 0, op_unimplemented },
    [0xb3] = { "OR E", 1, 4, 0, op_unimplemented },
    [0xb4] = { "OR H", 1, 4, 0, op_unimplemented },
    [0xb5] = { "OR L", 1, 4, 0, op_unimplemented },
    [0xb6] = { "OR (HL)", 1, 8, 0, op_unimplemented },
    [0xb7] = { "OR A", 1, 4, 0, op_unimplemented },
    [0xb8] = { "CP B", 1, 4, 0, op_unimplemented },
    [0xb9] = { "CP C", 1, 4, 0, op_unimplemented },
    [0xba] = { "CP D", 1, 4, 0, op_unimplemented },
    [0xbb] = { "CP E", 1, 4, 0, op_unimplemented },
    [0xbc] = { "CP H", 1, 4, 0, op_unimplemented },
    [0xbd] = { "CP L", 1, 4, 0, op_unimplemented },
    [0xbe] = { "CP (HL)", 1, 8, 0, op_unimplemented },
    [0xbf] = { "CP A", 1, 4, 0, op_unimplemented },
    [0xc0] = { "RET NZ", 1, 20, 8, op_unimplemented },
    [0xc1] = { "POP BC", 1, 12, 0, op_unimplemented },
    [0xc2] = { "JP NZ, a16", 3, 16, 12, op_unimplemented },
    [0xc3] = { "JP a16", 3, 16, 0, op_unimplemented },
    [0xc4] = { "CALL NZ, a16", 3, 24, 12, op_unimplemented },
    [0xc5] = { "PUSH BC", 1, 16, 0, op_unimplemented },
    [0xc6] = { "ADD A, d8", 2, 8, 0, op_unimplemented },
    [0xc7] = { "RST 00H", 1, 16, 0, op_unimplemented },
    [0xc8] = { "RET Z", 1, 20, 8, op_unimplemented },
    [0xc9] = { "RET", 1, 16, 0, op_unimplemented },
    [0xca] = { "JP Z, a16", 3, 16, 12, op_unimplemented },
    [0xcb] = { "PREFIX CB", 1, 4, 0, op_cb_prefix },
    [0xcc] = { "CALL Z, a16", 3, 24, 12, op_unimplemented },
    [0xcd] = { "CALL a16", 3, 24, 0, op_unimplemented },
    [0xce] = { "ADC A, d8", 2, 8, 0, op_unimplemented },
    [0xcf] = { "RST 08H", 1, 16, 0, op_unimplemented },
    [0xd0] = { "RET NC", 1, 20, 8, op_unimplemented },
    [0xd1] = { "POP DE", 1, 12, 0, op_unimplemented },
    [0xd2] = { "JP NC, a16", 3, 16, 12, op_unimplemented },
    [0xd4] = { "CALL NC, a16", 3, 24, 12, op_unimplemented },
    [0xd5] = { "PUSH DE", 1, 16, 0, op_unimplemented },
    [0xd6] = { "SUB d8", 2, 8, 0, op_unimplemented },
    [0xd7] = { "RST 10H", 1, 16, 0, op_unimplemented },
    [0xd8] = { "RET C", 1, 20, 8, op_unimplemented },
    [0xd9] = { "RETI", 1, 16, 0, op_unimplemented },
    [0xda] = { "JP C, a16", 3, 16, 12, op_unimplemented },
    [0xdc] = { "CALL C, a16", 3, 24, 12, op_unimplemented },
    [0xde] = { "SBC A, d8", 2, 8, 0, op_unimplemented },
    [0xdf] = { "RST 18H", 1, 16, 0, op_unimplemented },
    [0xe0] = { "LDH (a8), A", 2, 12, 0, op_unimplemented },
    [0xe1] = { "POP HL", 1, 12, 0, op_unimplemented },
    [0xe2] = { "LD (C), A", 1, 8, 0, op_unimplemented },
    [0xe5] = { "PUSH HL", 1, 16, 0, op_unimplemented },
    [0xe6] = { "AND d8", 2, 8, 0, op_unimplemented },
    [0xe7] = { "RST 20H", 1, 16, 0, op_unimplemented },
    [0xe8] = { "ADD SP, r8", 2, 16, 0, op_unimplemented },
    [0xe9] = { "JP (HL)", 1, 4, 0, op_unimplemented },
    [0xea] = { "LD (a16), A", 3, 16, 0, op_unimplemented },
    [0xee] = { "XOR d8", 2, 8, 0, op_unimplemented },
    [0xef] = { "RST 28H", 1, 16, 0, op_unimplemented },
    [0xf0] = { "LDH A, (a8)", 2, 12, 0, op_unimplemented },
    [0xf1] = { "POP AF", 1, 12, 0, op_unimplemented },
    [0xf2] = { "LD A, (C)", 1, 8, 0, op_unimplemented },
    [0xf3] = { "DI", 1, 4, 0, op_unimplemented },
    [0xf5] = { "PUSH AF", 1, 16, 0, op_unimplemented },
    [0xf6] = { "OR d8", 2, 8, 0, op_unimplemented },
    [0xf7] = { "RST 30H", 1, 16, 0, op_unimplemented },
    [0xf8] = { "LD HL, SP+r8", 2, 12, 0, op_unimplemented },
    [0xf9] = { "LD SP, HL", 1, 8, 0, op_unimplemented },
    [0xfa] = { "LD A, (a16)", 3, 16, 0, op_unimplemented },
    [0xfb] = { "EI", 1, 4, 0, op_unimplemented },
    [0xfe] = { "CP d8", 2, 8, 0, op_unimplemented },
    [0xff] = { "RST 38H", 1, 16, 0, op_unimplemented },
};

enum { FLAG_Z = 0x80u, FLAG_N = 0x40u, FLAG_H = 0x20u, FLAG_C = 0x10u };

static uint8_t read8(CPU *cpu, MMU *mmu, uint16_t addr)
{
    (void)cpu;
    return mmu_read_byte(mmu, addr);
}

static void write8(CPU *cpu, MMU *mmu, uint16_t addr, uint8_t value)
{
    (void)cpu;
    mmu_write_byte(mmu, addr, value);
}

static uint16_t read16(CPU *cpu, MMU *mmu, uint16_t addr)
{
    uint16_t low = read8(cpu, mmu, addr);
    uint16_t high = read8(cpu, mmu, (uint16_t)(addr + 1u));
    return (uint16_t)(low | (uint16_t)(high << 8));
}

static void write16(CPU *cpu, MMU *mmu, uint16_t addr, uint16_t value)
{
    write8(cpu, mmu, addr, (uint8_t)value);
    write8(cpu, mmu, (uint16_t)(addr + 1u), (uint8_t)(value >> 8));
}

static uint16_t *register_pair(CPU *cpu, uint8_t index)
{
    switch (index) {
    case 0: return &cpu->BC_reg.bc;
    case 1: return &cpu->DE_reg.de;
    case 2: return &cpu->HL_reg.hl;
    default: return &cpu->sp;
    }
}

static uint8_t get_register(CPU *cpu, MMU *mmu, uint8_t index)
{
    switch (index) {
    case 0: return cpu->BC_reg.bytes.b;
    case 1: return cpu->BC_reg.bytes.c;
    case 2: return cpu->DE_reg.bytes.d;
    case 3: return cpu->DE_reg.bytes.e;
    case 4: return cpu->HL_reg.bytes.h;
    case 5: return cpu->HL_reg.bytes.l;
    case 6: return read8(cpu, mmu, cpu->HL_reg.hl);
    default: return cpu->AF_reg.bytes.a;
    }
}

static void set_register(CPU *cpu, MMU *mmu, uint8_t index, uint8_t value)
{
    switch (index) {
    case 0: cpu->BC_reg.bytes.b = value; break;
    case 1: cpu->BC_reg.bytes.c = value; break;
    case 2: cpu->DE_reg.bytes.d = value; break;
    case 3: cpu->DE_reg.bytes.e = value; break;
    case 4: cpu->HL_reg.bytes.h = value; break;
    case 5: cpu->HL_reg.bytes.l = value; break;
    case 6: write8(cpu, mmu, cpu->HL_reg.hl, value); break;
    default: cpu->AF_reg.bytes.a = value; break;
    }
}

static void set_zero_flag(CPU *cpu, uint8_t result)
{
    cpu->AF_reg.bytes.f &= ~(FLAG_Z | FLAG_N | FLAG_H | FLAG_C);
    if (result == 0u) cpu->AF_reg.bytes.f |= FLAG_Z;
}

static void inc_register(CPU *cpu, MMU *mmu, uint8_t index)
{
    uint8_t value = get_register(cpu, mmu, index);
    uint8_t result = (uint8_t)(value + 1u);
    uint8_t carry = cpu->AF_reg.bytes.f & FLAG_C;
    set_register(cpu, mmu, index, result);
    cpu->AF_reg.bytes.f = (uint8_t)(carry | (result == 0u ? FLAG_Z : 0u) |
                                    ((value & 0x0Fu) == 0x0Fu ? FLAG_H : 0u));
}

static void dec_register(CPU *cpu, MMU *mmu, uint8_t index)
{
    uint8_t value = get_register(cpu, mmu, index);
    uint8_t result = (uint8_t)(value - 1u);
    uint8_t carry = cpu->AF_reg.bytes.f & FLAG_C;
    set_register(cpu, mmu, index, result);
    cpu->AF_reg.bytes.f = (uint8_t)(carry | FLAG_N |
                                    (result == 0u ? FLAG_Z : 0u) |
                                    ((value & 0x0Fu) == 0u ? FLAG_H : 0u));
}

static void add_a(CPU *cpu, uint8_t value, bool carry_in)
{
    uint8_t a = cpu->AF_reg.bytes.a;
    uint16_t carry = carry_in && (cpu->AF_reg.bytes.f & FLAG_C) ? 1u : 0u;
    uint16_t result = (uint16_t)a + value + carry;
    cpu->AF_reg.bytes.f = 0u;
    if ((uint8_t)result == 0u) cpu->AF_reg.bytes.f |= FLAG_Z;
    if (((a & 0x0Fu) + (value & 0x0Fu) + carry) > 0x0Fu) cpu->AF_reg.bytes.f |= FLAG_H;
    if (result > 0xFFu) cpu->AF_reg.bytes.f |= FLAG_C;
    cpu->AF_reg.bytes.a = (uint8_t)result;
}

static void sub_a(CPU *cpu, uint8_t value, bool carry_in)
{
    uint8_t a = cpu->AF_reg.bytes.a;
    uint8_t carry = (uint8_t)(carry_in &&
                              (cpu->AF_reg.bytes.f & FLAG_C) ? 1u : 0u);
    uint16_t result = (uint16_t)((uint16_t)a -
                                 (uint16_t)value - carry);
    cpu->AF_reg.bytes.f = FLAG_N;
    if ((uint8_t)result == 0u) cpu->AF_reg.bytes.f |= FLAG_Z;
    if ((a & 0x0Fu) < ((value & 0x0Fu) + carry)) {
        cpu->AF_reg.bytes.f |= FLAG_H;
    }
    if (a < (uint16_t)value + carry) cpu->AF_reg.bytes.f |= FLAG_C;
    cpu->AF_reg.bytes.a = (uint8_t)result;
}

static void and_a(CPU *cpu, uint8_t value)
{
    cpu->AF_reg.bytes.a &= value;
    cpu->AF_reg.bytes.f = FLAG_H | (cpu->AF_reg.bytes.a == 0u ? FLAG_Z : 0u);
}

static void xor_a(CPU *cpu, uint8_t value)
{
    cpu->AF_reg.bytes.a ^= value;
    set_zero_flag(cpu, cpu->AF_reg.bytes.a);
}

static void or_a(CPU *cpu, uint8_t value)
{
    cpu->AF_reg.bytes.a |= value;
    set_zero_flag(cpu, cpu->AF_reg.bytes.a);
}

static void cp_a(CPU *cpu, uint8_t value)
{
    uint8_t a = cpu->AF_reg.bytes.a;
    sub_a(cpu, value, false);
    cpu->AF_reg.bytes.a = a;
}

static void push(CPU *cpu, MMU *mmu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(cpu, mmu, cpu->sp, value);
}

static uint16_t pop(CPU *cpu, MMU *mmu)
{
    uint16_t value = read16(cpu, mmu, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void op_cb_prefix(CPU *cpu, MMU *mmu, uint8_t opcode)
{
    uint8_t cb_opcode = mmu_read_byte(mmu, cpu->pc++);
    uint8_t operation = (uint8_t)(cb_opcode >> 3);
    uint8_t reg_index = (uint8_t)(cb_opcode & 7u);
    uint8_t value = get_register(cpu, mmu, reg_index);
    uint8_t result = value;
    bool carry = (cpu->AF_reg.bytes.f & FLAG_C) != 0u;

    (void)opcode;
    if (cb_opcode < 0x40u) {
        uint8_t subop = (uint8_t)(cb_opcode >> 3);
        if (subop == 0u) {
            bool old_high = (value & 0x80u) != 0u;
            result = (uint8_t)((value << 1) | (old_high ? 1u : 0u));
            cpu->AF_reg.bytes.f = old_high ? FLAG_C : 0u;
        } else if (subop == 1u) {
            bool old_low = (value & 1u) != 0u;
            result = (uint8_t)((value >> 1) | (old_low ? 0x80u : 0u));
            cpu->AF_reg.bytes.f = old_low ? FLAG_C : 0u;
        } else if (subop == 2u) {
            bool old_high = (value & 0x80u) != 0u;
            result = (uint8_t)((value << 1) | (carry ? 1u : 0u));
            cpu->AF_reg.bytes.f = old_high ? FLAG_C : 0u;
        } else if (subop == 3u) {
            bool old_low = (value & 1u) != 0u;
            result = (uint8_t)((value >> 1) | (carry ? 0x80u : 0u));
            cpu->AF_reg.bytes.f = old_low ? FLAG_C : 0u;
        } else if (subop == 4u) {
            bool old_high = (value & 0x80u) != 0u;
            result = (uint8_t)(value << 1);
            cpu->AF_reg.bytes.f = old_high ? FLAG_C : 0u;
        } else if (subop == 5u) {
            bool old_low = (value & 1u) != 0u;
            result = (uint8_t)((value >> 1) | (value & 0x80u));
            cpu->AF_reg.bytes.f = old_low ? FLAG_C : 0u;
        } else if (subop == 6u) {
            result = (uint8_t)(((value & 0x0Fu) << 4) | ((value & 0xF0u) >> 4));
            cpu->AF_reg.bytes.f = 0u;
        } else if (subop == 7u) {
            bool old_low = (value & 1u) != 0u;
            result = (uint8_t)(value >> 1);
            cpu->AF_reg.bytes.f = old_low ? FLAG_C : 0u;
        }
        if (result == 0u) cpu->AF_reg.bytes.f |= FLAG_Z;
        set_register(cpu, mmu, reg_index, result);
    } else if (cb_opcode < 0x80u) {
        uint8_t bit = (uint8_t)((operation - 8u) & 7u);
        if ((value & (uint8_t)(1u << bit)) == 0u) cpu->AF_reg.bytes.f |= FLAG_Z;
        else cpu->AF_reg.bytes.f &= ~FLAG_Z;
        cpu->AF_reg.bytes.f &= ~FLAG_N;
        cpu->AF_reg.bytes.f |= FLAG_H;
    } else if (cb_opcode < 0xC0u) {
        uint8_t bit = (uint8_t)((operation - 16u) & 7u);
        set_register(cpu, mmu, reg_index, (uint8_t)(value & (uint8_t)~(1u << bit)));
    } else {
        uint8_t bit = (uint8_t)((operation - 24u) & 7u);
        set_register(cpu, mmu, reg_index, (uint8_t)(value | (uint8_t)(1u << bit)));
    }
    if (reg_index == 6u) {
        cpu->cycles += (cb_opcode >= 0x40u && cb_opcode < 0x80u) ? 12u : 16u;
    } else {
        cpu->cycles += 8u;
    }
}

bool cpu_execute_opcode(CPU *cpu, MMU *mmu, uint8_t opcode)
{
    const Instruction *instruction = &opcodes[opcode];

    if (instruction->execute == NULL && opcode != 0xcb) {
        fprintf(stderr, "Illegal opcode: 0x%02X at PC: 0x%04X\n",
                opcode, (uint16_t)(cpu->pc - 1));
        return false;
    }

    cpu->branch_taken = true;
    if (opcode >= 0x40u && opcode <= 0x7fu && opcode != 0x76u) {
        uint8_t source = (uint8_t)(opcode & 7u);
        uint8_t destination = (uint8_t)((opcode - 0x40u) >> 3);
        set_register(cpu, mmu, destination, get_register(cpu, mmu, source));
    } else if (opcode >= 0x80u && opcode <= 0xbfu) {
        uint8_t operation = (uint8_t)((opcode - 0x80u) >> 3);
        uint8_t value = get_register(cpu, mmu, (uint8_t)(opcode & 7u));
        switch (operation) {
        case 0: add_a(cpu, value, false); break;
        case 1: add_a(cpu, value, true); break;
        case 2: sub_a(cpu, value, false); break;
        case 3: sub_a(cpu, value, true); break;
        case 4: and_a(cpu, value); break;
        case 5: xor_a(cpu, value); break;
        case 6: or_a(cpu, value); break;
        default: cp_a(cpu, value); break;
        }
    } else {
    switch (opcode) {
    case 0x00: case 0x40: case 0x49: case 0x52: case 0x5b:
    case 0x64: case 0x6d: case 0x77: case 0x7f:
        if (opcode >= 0x40u && opcode != 0x76u) {
            uint8_t source = (uint8_t)((opcode - 0x40u) & 7u);
            uint8_t destination = (uint8_t)(((opcode - 0x40u) >> 3) & 7u);
            set_register(cpu, mmu, destination, get_register(cpu, mmu, source));
        }
        break;
    case 0x01: case 0x11: case 0x21: case 0x31: {
        uint16_t value = read16(cpu, mmu, cpu->pc);
        cpu->pc = (uint16_t)(cpu->pc + 2u);
        *register_pair(cpu, (uint8_t)(opcode >> 4)) = value;
        break;
    }
    case 0x02: write8(cpu, mmu, cpu->BC_reg.bc, cpu->AF_reg.bytes.a); break;
    case 0x12: write8(cpu, mmu, cpu->DE_reg.de, cpu->AF_reg.bytes.a); break;
    case 0x22: write8(cpu, mmu, cpu->HL_reg.hl++, cpu->AF_reg.bytes.a); break;
    case 0x32: write8(cpu, mmu, cpu->HL_reg.hl--, cpu->AF_reg.bytes.a); break;
    case 0x0a: cpu->AF_reg.bytes.a = read8(cpu, mmu, cpu->BC_reg.bc); break;
    case 0x1a: cpu->AF_reg.bytes.a = read8(cpu, mmu, cpu->DE_reg.de); break;
    case 0x2a: cpu->AF_reg.bytes.a = read8(cpu, mmu, cpu->HL_reg.hl++); break;
    case 0x3a: cpu->AF_reg.bytes.a = read8(cpu, mmu, cpu->HL_reg.hl--); break;
    case 0x03: cpu->BC_reg.bc++; break;
    case 0x13: cpu->DE_reg.de++; break;
    case 0x23: cpu->HL_reg.hl++; break;
    case 0x33: cpu->sp++; break;
    case 0x0b: cpu->BC_reg.bc--; break;
    case 0x1b: cpu->DE_reg.de--; break;
    case 0x2b: cpu->HL_reg.hl--; break;
    case 0x3b: cpu->sp--; break;
    case 0x04: inc_register(cpu, mmu, 0); break;
    case 0x05: dec_register(cpu, mmu, 0); break;
    case 0x0c: inc_register(cpu, mmu, 1); break;
    case 0x0d: dec_register(cpu, mmu, 1); break;
    case 0x14: inc_register(cpu, mmu, 2); break;
    case 0x15: dec_register(cpu, mmu, 2); break;
    case 0x1c: inc_register(cpu, mmu, 3); break;
    case 0x1d: dec_register(cpu, mmu, 3); break;
    case 0x24: inc_register(cpu, mmu, 4); break;
    case 0x25: dec_register(cpu, mmu, 4); break;
    case 0x2c: inc_register(cpu, mmu, 5); break;
    case 0x2d: dec_register(cpu, mmu, 5); break;
    case 0x3c: inc_register(cpu, mmu, 7); break;
    case 0x3d: dec_register(cpu, mmu, 7); break;
    case 0x34: inc_register(cpu, mmu, 6); break;
    case 0x35: dec_register(cpu, mmu, 6); break;
    case 0x06: cpu->BC_reg.bytes.b = read8(cpu, mmu, cpu->pc++); break;
    case 0x0e: cpu->BC_reg.bytes.c = read8(cpu, mmu, cpu->pc++); break;
    case 0x16: cpu->DE_reg.bytes.d = read8(cpu, mmu, cpu->pc++); break;
    case 0x1e: cpu->DE_reg.bytes.e = read8(cpu, mmu, cpu->pc++); break;
    case 0x26: cpu->HL_reg.bytes.h = read8(cpu, mmu, cpu->pc++); break;
    case 0x2e: cpu->HL_reg.bytes.l = read8(cpu, mmu, cpu->pc++); break;
    case 0x36: write8(cpu, mmu, cpu->HL_reg.hl, read8(cpu, mmu, cpu->pc++)); break;
    case 0x3e: cpu->AF_reg.bytes.a = read8(cpu, mmu, cpu->pc++); break;
    case 0x07: {
        bool carry = (cpu->AF_reg.bytes.a & 0x80u) != 0u;
        cpu->AF_reg.bytes.a = (uint8_t)((cpu->AF_reg.bytes.a << 1) |
                                        (carry ? 1u : 0u));
        cpu->AF_reg.bytes.f = carry ? FLAG_C : 0u;
        break;
    }
    case 0x17: {
        bool carry = (cpu->AF_reg.bytes.f & FLAG_C) != 0u;
        bool old_carry = (cpu->AF_reg.bytes.a & 0x80u) != 0u;
        cpu->AF_reg.bytes.a = (uint8_t)((cpu->AF_reg.bytes.a << 1) |
                                        (carry ? 1u : 0u));
        cpu->AF_reg.bytes.f = old_carry ? FLAG_C : 0u;
        break;
    }
    case 0x0f: {
        bool carry = (cpu->AF_reg.bytes.a & 1u) != 0u;
        cpu->AF_reg.bytes.a = (uint8_t)((cpu->AF_reg.bytes.a >> 1) |
                                        (carry ? 0x80u : 0u));
        cpu->AF_reg.bytes.f = carry ? FLAG_C : 0u;
        break;
    }
    case 0x1f: {
        bool carry = (cpu->AF_reg.bytes.f & FLAG_C) != 0u;
        bool old_carry = (cpu->AF_reg.bytes.a & 1u) != 0u;
        cpu->AF_reg.bytes.a = (uint8_t)((cpu->AF_reg.bytes.a >> 1) |
                                        (carry ? 0x80u : 0u));
        cpu->AF_reg.bytes.f = old_carry ? FLAG_C : 0u;
        break;
    }
    case 0x27: {
        uint8_t a = cpu->AF_reg.bytes.a;
        uint8_t correction = 0u;
        bool old_carry = (cpu->AF_reg.bytes.f & FLAG_C) != 0u;
        if (!(cpu->AF_reg.bytes.f & FLAG_N)) {
            if ((cpu->AF_reg.bytes.f & FLAG_H) ||
                (a & 0x0Fu) > 9u) correction |= 0x06u;
            if (old_carry || a > 0x99u) correction |= 0x60u;
            a = (uint8_t)(a + correction);
        } else {
            if (cpu->AF_reg.bytes.f & FLAG_H) correction |= 0x06u;
            if (old_carry) correction |= 0x60u;
            a = (uint8_t)(a - correction);
        }
        cpu->AF_reg.bytes.a = a;
        cpu->AF_reg.bytes.f &= (uint8_t)(FLAG_N | FLAG_C);
        if (!(cpu->AF_reg.bytes.f & FLAG_N) &&
            (correction & 0x60u) != 0u) {
            cpu->AF_reg.bytes.f |= FLAG_C;
        }
        cpu->AF_reg.bytes.f |= a == 0u ? FLAG_Z : 0u;
        cpu->AF_reg.bytes.f &= (uint8_t)~FLAG_H;
        break;
    }
    case 0x2f: cpu->AF_reg.bytes.a ^= 0xFFu; cpu->AF_reg.bytes.f |= FLAG_N | FLAG_H; break;
    case 0x37: cpu->AF_reg.bytes.f = (uint8_t)((cpu->AF_reg.bytes.f & (FLAG_Z | FLAG_C)) | FLAG_C); break;
    case 0x3f: cpu->AF_reg.bytes.f ^= FLAG_C; cpu->AF_reg.bytes.f &= ~(FLAG_N | FLAG_H); break;
    case 0x76:
        if (mmu != NULL && !cpu->ime &&
            (mmu->memory[0xFFFFu] & mmu->memory[0xFF0Fu]) != 0u) {
            cpu->halt_bug = true;
        } else {
            cpu->halted = true;
        }
        break;
    case 0x10:
        cpu->stopped = true;
        cpu->pc++;
        if (mmu != NULL && mmu->cgb_mode &&
            mmu->speed_switch_pending != 0u) {
            mmu->double_speed = !mmu->double_speed;
            mmu->speed_switch_pending = 0u;
            cpu->stopped = false;
        }
        break;
    case 0xcb: op_cb_prefix(cpu, mmu, opcode); break;
    case 0x09: case 0x19: case 0x29: case 0x39: {
        uint16_t value = *register_pair(cpu, (uint8_t)((opcode - 0x09u) >> 4));
        if (opcode == 0x39u) value = cpu->sp;
        uint32_t result = (uint32_t)cpu->HL_reg.hl + value;
        uint8_t old_high = (uint8_t)(cpu->HL_reg.hl >> 8);
        cpu->AF_reg.bytes.f &= FLAG_Z;
        if (((cpu->HL_reg.hl & 0x0FFFu) + (value & 0x0FFFu)) > 0x0FFFu) cpu->AF_reg.bytes.f |= FLAG_H;
        if (result > 0xFFFFu) cpu->AF_reg.bytes.f |= FLAG_C;
        cpu->HL_reg.hl = (uint16_t)result;
        (void)old_high;
        break;
    }
    case 0xe8: {
        int8_t offset = (int8_t)read8(cpu, mmu, cpu->pc++);
        uint16_t sp = cpu->sp;
        uint32_t result = (uint32_t)(sp + offset);
        cpu->AF_reg.bytes.f = 0u;
        if ((sp & 0x0Fu) + ((uint16_t)offset & 0x0Fu) > 0x0Fu) cpu->AF_reg.bytes.f |= FLAG_H;
        if ((sp & 0xFFu) + ((uint16_t)offset & 0xFFu) > 0xFFu) cpu->AF_reg.bytes.f |= FLAG_C;
        cpu->sp = (uint16_t)result;
        break;
    }
    case 0xf8: {
        int8_t offset = (int8_t)read8(cpu, mmu, cpu->pc++);
        uint16_t sp = cpu->sp;
        cpu->HL_reg.hl = (uint16_t)(sp + offset);
        cpu->AF_reg.bytes.f = 0u;
        if ((sp & 0x0Fu) + ((uint16_t)offset & 0x0Fu) > 0x0Fu) cpu->AF_reg.bytes.f |= FLAG_H;
        if ((sp & 0xFFu) + ((uint16_t)offset & 0xFFu) > 0xFFu) cpu->AF_reg.bytes.f |= FLAG_C;
        break;
    }
    case 0x08: {
        uint16_t addr = read16(cpu, mmu, cpu->pc);
        cpu->pc = (uint16_t)(cpu->pc + 2u);
        write16(cpu, mmu, addr, cpu->sp);
        break;
    }
    case 0xf9: cpu->sp = cpu->HL_reg.hl; break;
    case 0xc6: add_a(cpu, read8(cpu, mmu, cpu->pc++), false); break;
    case 0xce: add_a(cpu, read8(cpu, mmu, cpu->pc++), true); break;
    case 0xd6: sub_a(cpu, read8(cpu, mmu, cpu->pc++), false); break;
    case 0xde: sub_a(cpu, read8(cpu, mmu, cpu->pc++), true); break;
    case 0xe6: and_a(cpu, read8(cpu, mmu, cpu->pc++)); break;
    case 0xee: xor_a(cpu, read8(cpu, mmu, cpu->pc++)); break;
    case 0xf6: or_a(cpu, read8(cpu, mmu, cpu->pc++)); break;
    case 0xfe: cp_a(cpu, read8(cpu, mmu, cpu->pc++)); break;
    case 0x18: case 0x20: case 0x28: case 0x30: case 0x38: {
        int8_t offset = (int8_t)read8(cpu, mmu, cpu->pc++);
        bool take = true;
        if (opcode == 0x20) take = !(cpu->AF_reg.bytes.f & FLAG_Z);
        if (opcode == 0x28) take = (cpu->AF_reg.bytes.f & FLAG_Z) != 0u;
        if (opcode == 0x30) take = !(cpu->AF_reg.bytes.f & FLAG_C);
        if (opcode == 0x38) take = (cpu->AF_reg.bytes.f & FLAG_C) != 0u;
        cpu->branch_taken = take;
        if (take) cpu->pc = (uint16_t)(cpu->pc + offset);
        break;
    }
    case 0xc3: case 0xc2: case 0xca: case 0xd2: case 0xda: {
        uint16_t addr = read16(cpu, mmu, cpu->pc);
        cpu->pc = (uint16_t)(cpu->pc + 2u);
        bool take = true;
        if (opcode == 0xc2) take = !(cpu->AF_reg.bytes.f & FLAG_Z);
        if (opcode == 0xca) take = (cpu->AF_reg.bytes.f & FLAG_Z) != 0u;
        if (opcode == 0xd2) take = !(cpu->AF_reg.bytes.f & FLAG_C);
        if (opcode == 0xda) take = (cpu->AF_reg.bytes.f & FLAG_C) != 0u;
        cpu->branch_taken = take;
        if (take) cpu->pc = addr;
        break;
    }
    case 0xe9: cpu->pc = cpu->HL_reg.hl; break;
    case 0xcd: case 0xc4: case 0xcc: case 0xd4: case 0xdc: {
        uint16_t addr = read16(cpu, mmu, cpu->pc);
        cpu->pc = (uint16_t)(cpu->pc + 2u);
        bool take = true;
        if (opcode == 0xc4) take = !(cpu->AF_reg.bytes.f & FLAG_Z);
        if (opcode == 0xcc) take = (cpu->AF_reg.bytes.f & FLAG_Z) != 0u;
        if (opcode == 0xd4) take = !(cpu->AF_reg.bytes.f & FLAG_C);
        if (opcode == 0xdc) take = (cpu->AF_reg.bytes.f & FLAG_C) != 0u;
        cpu->branch_taken = take;
        if (take) { push(cpu, mmu, cpu->pc); cpu->pc = addr; }
        break;
    }
    case 0xc9: case 0xc0: case 0xc8: case 0xd0: case 0xd8: {
        bool take = true;
        if (opcode == 0xc0) take = !(cpu->AF_reg.bytes.f & FLAG_Z);
        if (opcode == 0xc8) take = (cpu->AF_reg.bytes.f & FLAG_Z) != 0u;
        if (opcode == 0xd0) take = !(cpu->AF_reg.bytes.f & FLAG_C);
        if (opcode == 0xd8) take = (cpu->AF_reg.bytes.f & FLAG_C) != 0u;
        cpu->branch_taken = take;
        if (take) cpu->pc = pop(cpu, mmu);
        break;
    }
    case 0xd9: cpu->pc = pop(cpu, mmu); cpu->ime = true; break;
    case 0xc1: cpu->BC_reg.bc = pop(cpu, mmu); break;
    case 0xd1: cpu->DE_reg.de = pop(cpu, mmu); break;
    case 0xe1: cpu->HL_reg.hl = pop(cpu, mmu); break;
    case 0xf1: cpu->AF_reg.af = pop(cpu, mmu) & 0xFFF0u; break;
    case 0xc5: push(cpu, mmu, cpu->BC_reg.bc); break;
    case 0xd5: push(cpu, mmu, cpu->DE_reg.de); break;
    case 0xe5: push(cpu, mmu, cpu->HL_reg.hl); break;
    case 0xf5: push(cpu, mmu, cpu->AF_reg.af); break;
    case 0xc7: push(cpu, mmu, cpu->pc); cpu->pc = 0x00u; break;
    case 0xcf: push(cpu, mmu, cpu->pc); cpu->pc = 0x08u; break;
    case 0xd7: push(cpu, mmu, cpu->pc); cpu->pc = 0x10u; break;
    case 0xdf: push(cpu, mmu, cpu->pc); cpu->pc = 0x18u; break;
    case 0xe7: push(cpu, mmu, cpu->pc); cpu->pc = 0x20u; break;
    case 0xef: push(cpu, mmu, cpu->pc); cpu->pc = 0x28u; break;
    case 0xf7: push(cpu, mmu, cpu->pc); cpu->pc = 0x30u; break;
    case 0xff: push(cpu, mmu, cpu->pc); cpu->pc = 0x38u; break;
    case 0xe0: write8(cpu, mmu, (uint16_t)(0xFF00u + read8(cpu, mmu, cpu->pc++)), cpu->AF_reg.bytes.a); break;
    case 0xf0: cpu->AF_reg.bytes.a = read8(cpu, mmu, (uint16_t)(0xFF00u + read8(cpu, mmu, cpu->pc++))); break;
    case 0xe2: write8(cpu, mmu, (uint16_t)(0xFF00u + cpu->BC_reg.bytes.c), cpu->AF_reg.bytes.a); break;
    case 0xf2: cpu->AF_reg.bytes.a = read8(cpu, mmu, (uint16_t)(0xFF00u + cpu->BC_reg.bytes.c)); break;
    case 0xea: {
        uint16_t addr = read16(cpu, mmu, cpu->pc);
        cpu->pc = (uint16_t)(cpu->pc + 2u);
        write8(cpu, mmu, addr, cpu->AF_reg.bytes.a);
        break;
    }
    case 0xfa: {
        uint16_t addr = read16(cpu, mmu, cpu->pc);
        cpu->pc = (uint16_t)(cpu->pc + 2u);
        cpu->AF_reg.bytes.a = read8(cpu, mmu, addr);
        break;
    }
    case 0xf3: cpu->ime = false; break;
    case 0xfb: cpu->ime = true; break;
    default:
        fprintf(stderr, "Illegal opcode: 0x%02X at PC: 0x%04X\n",
                opcode, (uint16_t)(cpu->pc - 1));
        return false;
    }
    }

    if (opcode != 0xcb) {
        uint32_t cycles = instruction->cycles;
        if (instruction->alternate_cycles != 0u && !cpu->branch_taken) {
            cycles = instruction->alternate_cycles;
        }
        cpu->cycles += cycles;
    }
    return true;
}

bool cpu_step(CPU *cpu, MMU *mmu)
{
    if (cpu != NULL && cpu->halted) {
        cpu->cycles += 4u;
        return true;
    }
    if (cpu != NULL && cpu->halt_bug) {
        uint8_t opcode = mmu_read_byte(mmu, cpu->pc);
        cpu->halt_bug = false;
        return cpu_execute_opcode(cpu, mmu, opcode);
    }
    uint8_t opcode = mmu_read_byte(mmu, cpu->pc++);
    return cpu_execute_opcode(cpu, mmu, opcode);
}
