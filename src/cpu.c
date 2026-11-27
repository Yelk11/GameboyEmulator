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
static void op_cb_00_rlc_b(CPU *cpu, MMU *mmu, uint8_t opcode);

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

static void op_cb_00_rlc_b(CPU *cpu, MMU *mmu, uint8_t opcode)
{
    (void)cpu;
    (void)mmu;
    (void)opcode;
}

static const Instruction cb_opcodes[256] = {
    [0x00] = { "RLC B", 2, 8, 0, op_cb_00_rlc_b },
};

static void op_cb_prefix(CPU *cpu, MMU *mmu, uint8_t opcode)
{
    uint8_t cb_opcode;
    const Instruction *instruction;

    (void)opcode;
    cb_opcode = mmu_read_byte(mmu, cpu->pc++);
    instruction = &cb_opcodes[cb_opcode];

    if (instruction->execute == NULL) {
        printf("Unimplemented CB-prefixed opcode: 0x%02X at PC: 0x%04X\n",
               cb_opcode, (uint16_t)(cpu->pc - 1));
        return;
    }

    instruction->execute(cpu, mmu, cb_opcode);
    cpu->cycles += instruction->cycles;
}

bool cpu_execute_opcode(CPU *cpu, MMU *mmu, uint8_t opcode)
{
    const Instruction *instruction = &opcodes[opcode];

    if (instruction->execute == NULL) {
        fprintf(stderr, "Illegal opcode: 0x%02X at PC: 0x%04X\n",
                opcode, (uint16_t)(cpu->pc - 1));
        return false;
    }

    instruction->execute(cpu, mmu, opcode);
    if (opcode != 0xcb) {
        cpu->cycles += instruction->cycles;
    }
    return true;
}

bool cpu_step(CPU *cpu, MMU *mmu)
{
    uint8_t opcode = mmu_read_byte(mmu, cpu->pc++);
    return cpu_execute_opcode(cpu, mmu, opcode);
}
