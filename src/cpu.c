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
    cpu->ime_enable_pending = false;
    cpu->halted = false;
    cpu->stopped = false;
    cpu->branch_taken = true;
    cpu->halt_bug = false;
}

static uint8_t opcode_cycles(CPU *cpu, uint8_t opcode)
{
    if (opcode == 0xcb) return 4u;

    switch (opcode) {
    case 0x00: case 0x07: case 0x0f: case 0x17: case 0x1f:
    case 0x27: case 0x2f: case 0x37: case 0x3f:
    case 0x76: case 0x10: case 0xf3: case 0xfb:
    return 4u;

    case 0x08: return 20u;
    case 0xe8: return 16u;
    case 0xf8: return 12u;
    case 0xf9: return 8u;

    case 0x18: return 12u;
    case 0x20: case 0x28: case 0x30: case 0x38:
        return cpu->branch_taken ? 12u : 8u;

    case 0xc0: case 0xc8: case 0xd0: case 0xd8:
        return cpu->branch_taken ? 20u : 8u;
    case 0xc9: case 0xd9: return 16u;

    case 0xe9: return 4u;
    case 0xc2: case 0xca: case 0xd2: case 0xda:
        return cpu->branch_taken ? 16u : 12u;
    case 0xc3: return 16u;

    case 0xc4: case 0xcc: case 0xd4: case 0xdc:
        return cpu->branch_taken ? 24u : 12u;
    case 0xcd: return 24u;

    case 0xc1: case 0xd1: case 0xe1: case 0xf1:
        return 12u;
    case 0xc5: case 0xd5: case 0xe5: case 0xf5:
        return 16u;

    case 0xe0: case 0xf0:
        return 12u;
    case 0xe2: case 0xf2:
        return 8u;
    case 0xea: case 0xfa:
        return 16u;

    case 0xc7: case 0xcf: case 0xd7: case 0xdf:
    case 0xe7: case 0xef: case 0xf7: case 0xff:
        return 16u;

    case 0x06: case 0x0e: case 0x16: case 0x1e:
    case 0x26: case 0x2e: case 0x3e:
    case 0x0a: case 0x1a: case 0x22: case 0x32:
    case 0x2a: case 0x3a:
    case 0xc6: case 0xce: case 0xd6: case 0xde:
    case 0xe6: case 0xee: case 0xf6: case 0xfe:
        return 8u;

    case 0x36: return 12u;

    case 0x01: case 0x11: case 0x21: case 0x31:
        return 12u;

    case 0x03: case 0x13: case 0x23: case 0x33:
    case 0x0b: case 0x1b: case 0x2b: case 0x3b:
    case 0x09: case 0x19: case 0x29: case 0x39:
        return 8u;

    case 0x04: case 0x05: case 0x0c: case 0x0d:
    case 0x14: case 0x15: case 0x1c: case 0x1d:
    case 0x24: case 0x25: case 0x2c: case 0x2d:
    case 0x3c: case 0x3d:
        return 4u;
    case 0x34: case 0x35:
        return 12u;

    case 0x02: case 0x12:
        return 8u;

    default:
        if (opcode >= 0x40u && opcode <= 0x7fu && opcode != 0x76u)
            return ((opcode & 7u) == 6u || ((opcode >> 3) & 7u) == 6u) ? 8u : 4u;
        if (opcode >= 0x80u && opcode <= 0xbfu)
            return (opcode & 7u) == 6u ? 8u : 4u;
        return 4u;
    }
}

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
        if(!cpu->ime &&
            (mmu_read_byte(mmu, 0xFFFFu) &
            mmu_read_byte(mmu, 0xFF0Fu) & 0x1Fu) != 0u) 
        {
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
    case 0xd9: cpu->pc = pop(cpu, mmu); cpu->ime = true; cpu->ime_enable_pending = false; break;
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
    case 0xf3: /* DI */
        cpu->ime = false;
        cpu->ime_enable_pending = false;
        break;
    case 0xfb: /* EI */
        cpu->ime_enable_pending = true;
        break;
    default:
        fprintf(stderr, "Illegal opcode: 0x%02X at PC: 0x%04X\n",
                opcode, (uint16_t)(cpu->pc - 1));
        return false;
    }
    }

    if (opcode != 0xcb) {
        cpu->cycles += opcode_cycles(cpu, opcode);
    }
    return true;
}

bool cpu_step(CPU *cpu, MMU *mmu)
{
    if (cpu == NULL || mmu == NULL) {
        return false;
    }

    /*
     * Remember whether EI was executed during the previous
     * instruction. It should take effect after this instruction.
     */
    bool enable_ime = cpu->ime_enable_pending;

    if (cpu->halted) {
        uint8_t ie = mmu_read_byte(mmu, 0xFFFFu);
        uint8_t flags = mmu_read_byte(mmu, 0xFF0Fu);
        uint8_t pending = ie & flags & 0x1Fu;

        if (pending == 0u) {
            cpu->cycles += 4u;
            return true;
        }

        cpu->halted = false;
        cpu->cycles += 4u;
        return true;
    }

    uint8_t opcode;

    if (cpu->halt_bug) {
        opcode = mmu_read_byte(mmu, cpu->pc);
        cpu->halt_bug = false;
    } else {
        opcode = mmu_read_byte(mmu, cpu->pc++);
    }

    bool success = cpu_execute_opcode(cpu, mmu, opcode);

    if (!success) {
        return false;
    }

    /*
     * Apply the delayed EI after the following instruction.
     * DI cancels a pending EI.
     */
    if (enable_ime && opcode != 0xF3u) {
        cpu->ime = true;
        cpu->ime_enable_pending = false;
    }

    return true;
}