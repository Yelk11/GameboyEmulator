#include "mmu.h"


MMU *mmu_init(void)
{
    MMU *mmu = calloc(1, sizeof(MMU));
    return mmu; // Returns NULL if allocation fails
}

void mmu_destroy(MMU *mmu)
{
    free(mmu);
}

bool mmu_load_rom(MMU *mmu, const char *rom_name)
{
    FILE *fp;
    long rom_size;
    size_t bytes_read;

    if (mmu == NULL || rom_name == NULL) {
        fprintf(stderr, "Invalid MMU or ROM filename\n");
        return false;
    }

    fp = fopen(rom_name, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open ROM file: %s\n", rom_name);
        return false;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "Failed to determine ROM size: %s\n", rom_name);
        fclose(fp);
        return false;
    }

    rom_size = ftell(fp);
    if (rom_size < 0) {
        fprintf(stderr, "Failed to determine ROM size: %s\n", rom_name);
        fclose(fp);
        return false;
    }

    if ((unsigned long)rom_size > BUFFER_SIZE) {
        fprintf(stderr, "ROM is too large: %ld bytes\n", rom_size);
        fclose(fp);
        return false;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Failed to rewind ROM: %s\n", rom_name);
        fclose(fp);
        return false;
    }

    bytes_read = fread(mmu->memory, 1, (size_t)rom_size, fp);
    if (bytes_read != (size_t)rom_size) {
        fprintf(stderr, "Failed to read ROM: %s\n", rom_name);
        fclose(fp);
        return false;
    }

    fclose(fp);
    return true;
}


uint8_t mmu_read_byte(MMU* mmu, uint16_t addr)
{
    return mmu->memory[addr];
}

void mmu_write_byte(MMU* mmu, uint16_t addr, uint8_t val)
{
    mmu->memory[addr] = val;
}

