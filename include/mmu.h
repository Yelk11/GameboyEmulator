#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#define BUFFER_SIZE 0x10000


typedef struct {
    uint8_t memory[BUFFER_SIZE]; // Full 64KB address space (0x0000 - 0xFFFF)
} MMU;

MMU *mmu_init(void);

bool mmu_load_rom(MMU *mmu, const char *rom_name);
void mmu_destroy(MMU *mmu);


uint8_t mmu_read_byte(MMU *mmu, uint16_t addr);

void mmu_write_byte(MMU *mmu, uint16_t addr, uint8_t val);


#endif