#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#define BUFFER_SIZE 0x10000
#define CARTRIDGE_ROM_MAX (8u * 1024u * 1024u)
#define CARTRIDGE_RAM_MAX (128u * 1024u)

typedef enum CartridgeType {
    CARTRIDGE_ROM_ONLY,
    CARTRIDGE_MBC1,
    CARTRIDGE_MBC2,
    CARTRIDGE_MBC3,
    CARTRIDGE_MBC5,
    CARTRIDGE_MMM01,
    CARTRIDGE_MBC6,
    CARTRIDGE_MBC7,
    CARTRIDGE_UNKNOWN
} CartridgeType;

typedef struct Cartridge {
    CartridgeType type;
    uint8_t *rom;
    size_t rom_size;
    uint8_t *ram;
    size_t ram_size;
    uint16_t rom_bank;
    uint8_t ram_bank;
    bool ram_enabled;
    bool rtc_latched;
    char save_path[1024];
    uint8_t rtc[5];
} Cartridge;

typedef struct {
    uint8_t memory[BUFFER_SIZE]; // Full 64KB address space (0x0000 - 0xFFFF)
    Cartridge cartridge;
    uint8_t dma_source;
    bool dma_active;
    uint16_t dma_remaining;
    uint8_t action_buttons;
    uint8_t dpad_buttons;
    uint8_t ppu_mode;
} MMU;

MMU *mmu_init(void);

bool mmu_load_rom(MMU *mmu, const char *rom_name);
void mmu_destroy(MMU *mmu);
bool mmu_save_battery(MMU *mmu);
bool mmu_load_battery(MMU *mmu);
const char *mmu_cartridge_type_name(const MMU *mmu);

uint8_t mmu_read_byte(MMU *mmu, uint16_t addr);

void mmu_write_byte(MMU *mmu, uint16_t addr, uint8_t val);


#endif