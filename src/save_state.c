#include <stdio.h>
#include <string.h>

#include "gb.h"

#define SAVE_STATE_MAGIC "GBSTATE1"
#define SAVE_STATE_VERSION 3u
#define SAVE_STATE_SLOT_MAX 9u

typedef struct SaveHeader {
    char magic[8];
    uint32_t version;
    uint32_t rom_size;
    uint32_t rom_hash;
    uint32_t ram_size;
    uint32_t reserved;
} SaveHeader;

static uint32_t rom_hash(const uint8_t *rom, size_t size)
{
    uint32_t hash = 2166136261u;
    size_t index;
    if (rom == NULL) return 0u;
    for (index = 0u; index < size; index++) {
        hash ^= rom[index];
        hash *= 16777619u;
    }
    return hash;
}

static bool state_path(const GB *gb, uint8_t slot, char *path, size_t path_size)
{
    size_t length;
    size_t copy_length;
    const char *dot;

    if (gb == NULL || gb->cartridge.rom_name == NULL ||
        slot == 0u || slot > SAVE_STATE_SLOT_MAX ||
        path == NULL || path_size == 0u) {
        return false;
    }

    length = strlen(gb->cartridge.rom_name);
    dot = strrchr(gb->cartridge.rom_name, '.');
    copy_length = dot != NULL && dot > gb->cartridge.rom_name
                  ? (size_t)(dot - gb->cartridge.rom_name) : length;
    if (copy_length + 8u >= path_size) return false;

    memcpy(path, gb->cartridge.rom_name, copy_length);
    snprintf(path + copy_length, path_size - copy_length,
             ".state%u", (unsigned)slot);
    return true;
}

static bool write_u32(FILE *fp, uint32_t value)
{
    return fwrite(&value, sizeof(value), 1u, fp) == 1u;
}

static bool read_u32(FILE *fp, uint32_t *value)
{
    return value != NULL && fread(value, sizeof(*value), 1u, fp) == 1u;
}

static bool write_bool(FILE *fp, bool value)
{
    uint8_t encoded = value ? 1u : 0u;
    return fwrite(&encoded, 1u, 1u, fp) == 1u;
}

static bool read_bool(FILE *fp, bool *value)
{
    uint8_t encoded;
    if (value == NULL || fread(&encoded, 1u, 1u, fp) != 1u) return false;
    *value = encoded != 0u;
    return true;
}

bool gb_save_state(GB *gb, uint8_t slot)
{
    char path[1024];
    FILE *fp;
    SaveHeader header;

    if (gb == NULL || gb->cpu == NULL || gb->mmu == NULL ||
        gb->ppu == NULL || !state_path(gb, slot, path, sizeof(path))) {
        return false;
    }

    memset(&header, 0, sizeof(header));
    memcpy(header.magic, SAVE_STATE_MAGIC, 8u);
    header.version = SAVE_STATE_VERSION;
    header.rom_size = (uint32_t)gb->mmu->cartridge.rom_size;
    header.rom_hash = rom_hash(gb->mmu->cartridge.rom,
                               gb->mmu->cartridge.rom_size);
    header.ram_size = (uint32_t)gb->mmu->cartridge.ram_size;

    fp = fopen(path, "wb");
    if (fp == NULL) return false;

    if (fwrite(&header, sizeof(header), 1u, fp) != 1u ||
        !write_u32(fp, gb->total_t_cycles >> 32) ||
        !write_u32(fp, (uint32_t)gb->total_t_cycles) ||
        !write_u32(fp, (uint32_t)gb->frame_number) ||
        !write_u32(fp, gb->cpu->AF_reg.af) ||
        !write_u32(fp, gb->cpu->BC_reg.bc) ||
        !write_u32(fp, gb->cpu->DE_reg.de) ||
        !write_u32(fp, gb->cpu->HL_reg.hl) ||
        !write_u32(fp, gb->cpu->sp) ||
        !write_u32(fp, gb->cpu->pc) ||
        !write_u32(fp, gb->cpu->cycles) ||
        !write_bool(fp, gb->cpu->ime) ||
        !write_bool(fp, gb->cpu->halted) ||
        !write_bool(fp, gb->cpu->stopped) ||
        !write_bool(fp, gb->cpu->branch_taken) ||
        fwrite(gb->mmu->memory, sizeof(gb->mmu->memory), 1u, fp) != 1u ||
        !write_u32(fp, gb->mmu->cartridge.type) ||
        !write_u32(fp, (uint32_t)gb->mmu->cartridge.rom_bank) ||
        !write_u32(fp, gb->mmu->cartridge.ram_bank) ||
        !write_bool(fp, gb->mmu->cartridge.ram_enabled) ||
        !write_bool(fp, gb->mmu->cartridge.rtc_latched) ||
        fwrite(gb->mmu->cartridge.rtc, sizeof(gb->mmu->cartridge.rtc), 1u, fp) != 1u ||
        (gb->mmu->cartridge.ram != NULL &&
         fwrite(gb->mmu->cartridge.ram, gb->mmu->cartridge.ram_size, 1u, fp) != 1u) ||
        !write_u32(fp, gb->timer.div) ||
        !write_u32(fp, gb->timer.div_counter) ||
        !write_u32(fp, gb->timer.timer_counter) ||
        !write_u32(fp, gb->timer.tima) ||
        !write_u32(fp, gb->timer.tma) ||
        !write_u32(fp, gb->timer.tac) ||
        !write_bool(fp, gb->timer.overflow_pending) ||
        !write_u32(fp, gb->interrupt.ie) ||
        !write_u32(fp, gb->interrupt.if_) ||
        !write_bool(fp, gb->interrupt.ime) ||
        !write_u32(fp, gb->joypad.action_buttons) ||
        !write_u32(fp, gb->joypad.dpad_buttons) ||
        !write_bool(fp, gb->joypad.interrupt_pending) ||
        !write_bool(fp, gb->dma.active) ||
        !write_u32(fp, gb->dma.source) ||
        !write_u32(fp, gb->dma.destination) ||
        !write_u32(fp, gb->dma.remaining) ||
        !write_u32(fp, gb->serial.sb) ||
        !write_u32(fp, gb->serial.sc) ||
        !write_u32(fp, gb->serial.transfer_counter) ||
        !write_u32(fp, gb->ppu->mode) ||
        !write_u32(fp, gb->ppu->ly) ||
        !write_u32(fp, gb->ppu->line_dot) ||
        !write_u32(fp, gb->ppu->frame_cycle) ||
        fwrite(gb->ppu->framebuffer, sizeof(gb->ppu->framebuffer), 1u, fp) != 1u ||
        !write_bool(fp, gb->mmu->dma_active) ||
        !write_u32(fp, gb->mmu->dma_source) ||
        !write_u32(fp, gb->mmu->dma_remaining) ||
        !write_u32(fp, gb->mmu->ppu_mode) ||
        !write_bool(fp, gb->ppu->stat_irq_line) ||
        !write_u32(fp, gb->ppu->window_line)) {
        fclose(fp);
        remove(path);
        return false;
    }

    fclose(fp);
    return true;
}

bool gb_load_state(GB *gb, uint8_t slot)
{
    char path[1024];
    FILE *fp;
    SaveHeader header;
    uint32_t high, low, frame;
    uint32_t ram_bank, type, rom_bank;
    uint32_t af, bc, de, hl, sp, pc, cpu_cycles;
    uint32_t div, div_counter, timer_counter, tima, tma, tac;
    uint32_t ie, if_, action_buttons, dpad_buttons;
    uint32_t dma_source, dma_destination, dma_remaining;
    uint32_t sb, sc, serial_counter;
    uint32_t ppu_mode, ly, line_dot, frame_cycle;
    uint32_t dma_source_reg, dma_remaining_reg, ppu_mode_reg;
    bool stat_irq_line = false;
    uint32_t window_line = 0u;

    if (gb == NULL || gb->cpu == NULL || gb->mmu == NULL ||
        gb->ppu == NULL || !state_path(gb, slot, path, sizeof(path))) {
        return false;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) return false;
    if (fread(&header, sizeof(header), 1u, fp) != 1u ||
        memcmp(header.magic, SAVE_STATE_MAGIC, 8u) != 0 ||
        (header.version < 1u || header.version > SAVE_STATE_VERSION) ||
        header.rom_size != (uint32_t)gb->mmu->cartridge.rom_size ||
        header.rom_hash != rom_hash(gb->mmu->cartridge.rom,
                                    gb->mmu->cartridge.rom_size) ||
        header.ram_size != (uint32_t)gb->mmu->cartridge.ram_size) {
        fclose(fp);
        return false;
    }

    if (!read_u32(fp, &high) || !read_u32(fp, &low) ||
        !read_u32(fp, &frame) ||
        !read_u32(fp, &af) || !read_u32(fp, &bc) ||
        !read_u32(fp, &de) || !read_u32(fp, &hl) ||
        !read_u32(fp, &sp) || !read_u32(fp, &pc) ||
        !read_u32(fp, &cpu_cycles) ||
        !read_bool(fp, &gb->cpu->ime) ||
        !read_bool(fp, &gb->cpu->halted) ||
        !read_bool(fp, &gb->cpu->stopped) ||
        !read_bool(fp, &gb->cpu->branch_taken) ||
        fread(gb->mmu->memory, sizeof(gb->mmu->memory), 1u, fp) != 1u ||
        !read_u32(fp, &type) ||
        !read_u32(fp, &rom_bank) ||
        !read_u32(fp, &ram_bank) ||
        !read_bool(fp, &gb->mmu->cartridge.ram_enabled) ||
        !read_bool(fp, &gb->mmu->cartridge.rtc_latched) ||
        fread(gb->mmu->cartridge.rtc, sizeof(gb->mmu->cartridge.rtc), 1u, fp) != 1u ||
        (gb->mmu->cartridge.ram != NULL &&
         fread(gb->mmu->cartridge.ram, gb->mmu->cartridge.ram_size, 1u, fp) != 1u) ||
        !read_u32(fp, &div) || !read_u32(fp, &div_counter) ||
        !read_u32(fp, &timer_counter) ||
        !read_u32(fp, &tima) || !read_u32(fp, &tma) ||
        !read_u32(fp, &tac) ||
        !read_bool(fp, &gb->timer.overflow_pending) ||
        !read_u32(fp, &ie) || !read_u32(fp, &if_) ||
        !read_bool(fp, &gb->interrupt.ime) ||
        !read_u32(fp, &action_buttons) ||
        !read_u32(fp, &dpad_buttons) ||
        !read_bool(fp, &gb->joypad.interrupt_pending) ||
        !read_bool(fp, &gb->dma.active) ||
        !read_u32(fp, &dma_source) ||
        !read_u32(fp, &dma_destination) ||
        !read_u32(fp, &dma_remaining) ||
        !read_u32(fp, &sb) || !read_u32(fp, &sc) ||
        !read_u32(fp, &serial_counter) ||
        !read_u32(fp, &ppu_mode) || !read_u32(fp, &ly) ||
        !read_u32(fp, &line_dot) || !read_u32(fp, &frame_cycle) ||
        fread(gb->ppu->framebuffer, sizeof(gb->ppu->framebuffer), 1u, fp) != 1u ||
        !read_bool(fp, &gb->mmu->dma_active) ||
        !read_u32(fp, &dma_source_reg) ||
        !read_u32(fp, &dma_remaining_reg) ||
        !read_u32(fp, &ppu_mode_reg) ||
        (header.version >= 2u && !read_bool(fp, &stat_irq_line)) ||
        (header.version >= 3u && !read_u32(fp, &window_line))) {
        fclose(fp);
        return false;
    }

    gb->total_t_cycles = ((uint64_t)high << 32) | low;
    gb->frame_number = frame;
    gb->cpu->AF_reg.af = (uint16_t)af;
    gb->cpu->BC_reg.bc = (uint16_t)bc;
    gb->cpu->DE_reg.de = (uint16_t)de;
    gb->cpu->HL_reg.hl = (uint16_t)hl;
    gb->cpu->sp = (uint16_t)sp;
    gb->cpu->pc = (uint16_t)pc;
    gb->cpu->cycles = cpu_cycles;
    gb->mmu->cartridge.ram_bank = (uint8_t)ram_bank;
    gb->mmu->cartridge.rom_bank = (uint16_t)rom_bank;
    gb->mmu->cartridge.type = (CartridgeType)type;
    gb->timer.div = (uint16_t)div;
    gb->timer.div_counter = div_counter;
    gb->timer.timer_counter = timer_counter;
    gb->timer.tima = (uint8_t)tima;
    gb->timer.tma = (uint8_t)tma;
    gb->timer.tac = (uint8_t)tac;
    gb->interrupt.ie = (uint8_t)ie;
    gb->interrupt.if_ = (uint8_t)if_;
    gb->joypad.action_buttons = (uint8_t)action_buttons;
    gb->joypad.dpad_buttons = (uint8_t)dpad_buttons;
    gb->dma.source = (uint16_t)dma_source;
    gb->dma.destination = (uint16_t)dma_destination;
    gb->dma.remaining = dma_remaining;
    gb->serial.sb = (uint8_t)sb;
    gb->serial.sc = (uint8_t)sc;
    gb->serial.transfer_counter = serial_counter;
    gb->ppu->mode = (PPU_MODE)ppu_mode;
    gb->ppu->ly = (uint8_t)ly;
    gb->ppu->line_dot = (uint16_t)line_dot;
    gb->ppu->frame_cycle = frame_cycle;
    gb->ppu->stat_irq_line = stat_irq_line;
    gb->ppu->window_line = (uint8_t)window_line;
    gb->mmu->dma_source = (uint8_t)dma_source_reg;
    gb->mmu->dma_remaining = (uint16_t)dma_remaining_reg;
    gb->mmu->ppu_mode = (uint8_t)ppu_mode_reg;
    gb->mmu->action_buttons = gb->joypad.action_buttons;
    gb->mmu->dpad_buttons = gb->joypad.dpad_buttons;
    fclose(fp);
    return true;
}
