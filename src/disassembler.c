#include "cpu.h"

#include <stdarg.h>
#include <stdio.h>

static const char *const registers8[8] = {
    "B", "C", "D", "E", "H", "L", "(HL)", "A"
};

static const char *const register_pairs[4] = {
    "BC", "DE", "HL", "SP"
};

static const char *const stack_pairs[4] = {
    "BC", "DE", "HL", "AF"
};

static const char *const conditions[4] = {
    "NZ", "Z", "NC", "C"
};

static const char *const alu_operations[8] = {
    "ADD A", "ADC A", "SUB", "SBC A", "AND", "XOR", "OR", "CP"
};

static bool format_text(char *text, size_t text_size,
                        const char *format, ...)
{
    va_list args;
    int written;

    va_start(args, format);
    written = vsnprintf(text, text_size, format, args);
    va_end(args);

    return written >= 0 && (size_t)written < text_size;
}

static uint8_t read_byte(MMU *mmu, uint16_t address, uint16_t offset)
{
    return mmu_read_byte(mmu, (uint16_t)(address + offset));
}

static uint16_t read_word(MMU *mmu, uint16_t address)
{
    uint16_t low = read_byte(mmu, address, 1u);
    uint16_t high = read_byte(mmu, address, 2u);
    return (uint16_t)(low | (high << 8u));
}

bool cpu_disassemble(MMU *mmu, uint16_t address, char *text,
                     size_t text_size, uint8_t *instruction_length)
{
    static const char *const cb_rotates[8] = {
        "RLC", "RRC", "RL", "RR", "SLA", "SRA", "SWAP", "SRL"
    };
    char decoded[32];
    uint8_t opcode;
    uint8_t length = 1u;
    bool formatted = false;

    if (text != NULL && text_size > 0u) text[0] = '\0';
    if (mmu == NULL || text == NULL || text_size == 0u ||
        instruction_length == NULL) {
        return false;
    }
    *instruction_length = 0u;
    opcode = mmu_read_byte(mmu, address);

    if (opcode == 0xCBu) {
        uint8_t extension = read_byte(mmu, address, 1u);
        uint8_t operation = (uint8_t)(extension >> 6u);
        uint8_t bit = (uint8_t)((extension >> 3u) & 7u);
        uint8_t reg = (uint8_t)(extension & 7u);

        length = 2u;
        if (operation == 0u) {
            formatted = format_text(decoded, sizeof(decoded), "%s %s",
                                    cb_rotates[bit], registers8[reg]);
        } else if (operation == 1u) {
            formatted = format_text(decoded, sizeof(decoded), "BIT %u,%s",
                                    bit, registers8[reg]);
        } else if (operation == 2u) {
            formatted = format_text(decoded, sizeof(decoded), "RES %u,%s",
                                    bit, registers8[reg]);
        } else {
            formatted = format_text(decoded, sizeof(decoded), "SET %u,%s",
                                    bit, registers8[reg]);
        }
    } else if (opcode >= 0x40u && opcode <= 0x7Fu) {
        if (opcode == 0x76u) {
            formatted = format_text(decoded, sizeof(decoded), "HALT");
        } else {
            uint8_t destination = (uint8_t)((opcode >> 3u) & 7u);
            uint8_t source = (uint8_t)(opcode & 7u);
            formatted = format_text(decoded, sizeof(decoded), "LD %s,%s",
                                    registers8[destination],
                                    registers8[source]);
        }
    } else if (opcode >= 0x80u && opcode <= 0xBFu) {
        uint8_t operation = (uint8_t)((opcode >> 3u) & 7u);
        uint8_t reg = (uint8_t)(opcode & 7u);
        formatted = format_text(decoded, sizeof(decoded), "%s,%s",
                                 alu_operations[operation], registers8[reg]);
    } else if ((opcode & 0xC7u) == 0x04u) {
        uint8_t reg = (uint8_t)((opcode >> 3u) & 7u);
        formatted = format_text(decoded, sizeof(decoded), "INC %s",
                                registers8[reg]);
    } else if ((opcode & 0xC7u) == 0x05u) {
        uint8_t reg = (uint8_t)((opcode >> 3u) & 7u);
        formatted = format_text(decoded, sizeof(decoded), "DEC %s",
                                registers8[reg]);
    } else if ((opcode & 0xC7u) == 0x06u) {
        uint8_t reg = (uint8_t)((opcode >> 3u) & 7u);
        uint8_t immediate = read_byte(mmu, address, 1u);
        length = 2u;
        formatted = format_text(decoded, sizeof(decoded), "LD %s,$%02X",
                                registers8[reg], immediate);
    } else if ((opcode & 0xCFu) == 0x01u) {
        uint8_t pair = (uint8_t)((opcode >> 4u) & 3u);
        length = 3u;
        formatted = format_text(decoded, sizeof(decoded), "LD %s,$%04X",
                                register_pairs[pair], read_word(mmu, address));
    } else if ((opcode & 0xCFu) == 0x03u) {
        uint8_t pair = (uint8_t)((opcode >> 4u) & 3u);
        formatted = format_text(decoded, sizeof(decoded), "INC %s",
                                register_pairs[pair]);
    } else if ((opcode & 0xCFu) == 0x0Bu) {
        uint8_t pair = (uint8_t)((opcode >> 4u) & 3u);
        formatted = format_text(decoded, sizeof(decoded), "DEC %s",
                                register_pairs[pair]);
    } else if ((opcode & 0xCFu) == 0x09u) {
        uint8_t pair = (uint8_t)((opcode >> 4u) & 3u);
        formatted = format_text(decoded, sizeof(decoded), "ADD HL,%s",
                                register_pairs[pair]);
    } else if ((opcode & 0xC7u) == 0xC6u) {
        uint8_t operation = (uint8_t)((opcode >> 3u) & 7u);
        uint8_t immediate = read_byte(mmu, address, 1u);
        length = 2u;
        formatted = format_text(decoded, sizeof(decoded), "%s,$%02X",
                                alu_operations[operation], immediate);
    } else {
        switch (opcode) {
        case 0x00: formatted = format_text(decoded, sizeof(decoded), "NOP"); break;
        case 0x02: formatted = format_text(decoded, sizeof(decoded), "LD (BC),A"); break;
        case 0x07: formatted = format_text(decoded, sizeof(decoded), "RLCA"); break;
        case 0x08:
            length = 3u;
            formatted = format_text(decoded, sizeof(decoded), "LD ($%04X),SP",
                                    read_word(mmu, address));
            break;
        case 0x0A: formatted = format_text(decoded, sizeof(decoded), "LD A,(BC)"); break;
        case 0x0F: formatted = format_text(decoded, sizeof(decoded), "RRCA"); break;
        case 0x10: length = 2u; formatted = format_text(decoded, sizeof(decoded), "STOP"); break;
        case 0x12: formatted = format_text(decoded, sizeof(decoded), "LD (DE),A"); break;
        case 0x17: formatted = format_text(decoded, sizeof(decoded), "RLA"); break;
        case 0x18:
        case 0x20:
        case 0x28:
        case 0x30:
        case 0x38: {
            uint8_t immediate = read_byte(mmu, address, 1u);
            uint16_t target = (uint16_t)((int32_t)address + 2 +
                                         (int32_t)(int8_t)immediate);
            length = 2u;
            if (opcode == 0x18u) {
                formatted = format_text(decoded, sizeof(decoded), "JR $%04X",
                                        target);
            } else {
                uint8_t condition = (uint8_t)((opcode >> 3u) & 3u);
                formatted = format_text(decoded, sizeof(decoded),
                                        "JR %s,$%04X",
                                        conditions[condition], target);
            }
            break;
        }
        case 0x1A: formatted = format_text(decoded, sizeof(decoded), "LD A,(DE)"); break;
        case 0x1F: formatted = format_text(decoded, sizeof(decoded), "RRA"); break;
        case 0x22: formatted = format_text(decoded, sizeof(decoded), "LD (HL+),A"); break;
        case 0x27: formatted = format_text(decoded, sizeof(decoded), "DAA"); break;
        case 0x2A: formatted = format_text(decoded, sizeof(decoded), "LD A,(HL+)"); break;
        case 0x2F: formatted = format_text(decoded, sizeof(decoded), "CPL"); break;
        case 0x32: formatted = format_text(decoded, sizeof(decoded), "LD (HL-),A"); break;
        case 0x37: formatted = format_text(decoded, sizeof(decoded), "SCF"); break;
        case 0x3A: formatted = format_text(decoded, sizeof(decoded), "LD A,(HL-)"); break;
        case 0x3F: formatted = format_text(decoded, sizeof(decoded), "CCF"); break;
        case 0xC0:
        case 0xC8:
        case 0xD0:
        case 0xD8: {
            uint8_t condition = (uint8_t)((opcode >> 3u) & 3u);
            formatted = format_text(decoded, sizeof(decoded), "RET %s",
                                    conditions[condition]);
            break;
        }
        case 0xC1:
        case 0xD1:
        case 0xE1:
        case 0xF1: {
            uint8_t pair = (uint8_t)((opcode >> 4u) & 3u);
            formatted = format_text(decoded, sizeof(decoded), "POP %s",
                                    stack_pairs[pair]);
            break;
        }
        case 0xC2:
        case 0xCA:
        case 0xD2:
        case 0xDA: {
            uint8_t condition = (uint8_t)((opcode >> 3u) & 3u);
            length = 3u;
            formatted = format_text(decoded, sizeof(decoded), "JP %s,$%04X",
                                    conditions[condition],
                                    read_word(mmu, address));
            break;
        }
        case 0xC3:
            length = 3u;
            formatted = format_text(decoded, sizeof(decoded), "JP $%04X",
                                    read_word(mmu, address));
            break;
        case 0xC4:
        case 0xCC:
        case 0xD4:
        case 0xDC: {
            uint8_t condition = (uint8_t)((opcode >> 3u) & 3u);
            length = 3u;
            formatted = format_text(decoded, sizeof(decoded), "CALL %s,$%04X",
                                    conditions[condition],
                                    read_word(mmu, address));
            break;
        }
        case 0xC5:
        case 0xD5:
        case 0xE5:
        case 0xF5: {
            uint8_t pair = (uint8_t)((opcode >> 4u) & 3u);
            formatted = format_text(decoded, sizeof(decoded), "PUSH %s",
                                    stack_pairs[pair]);
            break;
        }
        case 0xC7:
        case 0xCF:
        case 0xD7:
        case 0xDF:
        case 0xE7:
        case 0xEF:
        case 0xF7:
        case 0xFF:
            formatted = format_text(decoded, sizeof(decoded), "RST $%02X",
                                    opcode & 0x38u);
            break;
        case 0xC9: formatted = format_text(decoded, sizeof(decoded), "RET"); break;
        case 0xCD:
            length = 3u;
            formatted = format_text(decoded, sizeof(decoded), "CALL $%04X",
                                    read_word(mmu, address));
            break;
        case 0xD9: formatted = format_text(decoded, sizeof(decoded), "RETI"); break;
        case 0xE0:
        case 0xF0: {
            uint8_t immediate = read_byte(mmu, address, 1u);
            length = 2u;
            formatted = opcode == 0xE0u ?
                format_text(decoded, sizeof(decoded), "LDH ($FF%02X),A",
                            immediate) :
                format_text(decoded, sizeof(decoded), "LDH A,($FF%02X)",
                            immediate);
            break;
        }
        case 0xE2: formatted = format_text(decoded, sizeof(decoded), "LD (C),A"); break;
        case 0xE8: {
            int8_t offset = (int8_t)read_byte(mmu, address, 1u);
            length = 2u;
            formatted = format_text(decoded, sizeof(decoded), "ADD SP,%+d",
                                    (int)offset);
            break;
        }
        case 0xE9: formatted = format_text(decoded, sizeof(decoded), "JP (HL)"); break;
        case 0xEA:
            length = 3u;
            formatted = format_text(decoded, sizeof(decoded), "LD ($%04X),A",
                                    read_word(mmu, address));
            break;
        case 0xF2: formatted = format_text(decoded, sizeof(decoded), "LD A,(C)"); break;
        case 0xF3: formatted = format_text(decoded, sizeof(decoded), "DI"); break;
        case 0xF8: {
            int8_t offset = (int8_t)read_byte(mmu, address, 1u);
            length = 2u;
            formatted = format_text(decoded, sizeof(decoded), "LD HL,SP%+d",
                                    (int)offset);
            break;
        }
        case 0xF9: formatted = format_text(decoded, sizeof(decoded), "LD SP,HL"); break;
        case 0xFA:
            length = 3u;
            formatted = format_text(decoded, sizeof(decoded), "LD A,($%04X)",
                                    read_word(mmu, address));
            break;
        case 0xFB: formatted = format_text(decoded, sizeof(decoded), "EI"); break;
        default:
            formatted = format_text(decoded, sizeof(decoded), "ILLEGAL $%02X",
                                    opcode);
            break;
        }
    }

    if (!formatted) return false;
    if (!format_text(text, text_size, "%s", decoded)) {
        text[0] = '\0';
        return false;
    }
    *instruction_length = length;
    return true;
}
