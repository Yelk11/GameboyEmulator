#include "mmu.h"


MMU* mmu_init()
{
    MMU *mmu = calloc(1, sizeof(MMU));
    return mmu; // Returns NULL if allocation fails
}

void mmu_load_rom(MMU* mmu, char* rom_name)
{
    FILE *fp = fopen(rom_name, "rb"); // "rb" = Read Binary
    if (!fp) {
        perror("Failed to open ROM file");
    }

    // Read directly into the MMU memory array at the offset
    fread(&mmu->memory[0], 1, BUFFER_SIZE, fp);

    fclose(fp);
}


uint8_t mmu_read_byte(MMU* mmu, uint16_t addr)
{
    return mmu->memory[addr];
}

void mmu_write_byte(MMU* mmu, uint16_t addr, uint8_t val)
{
    mmu->memory[addr] = val;
}

