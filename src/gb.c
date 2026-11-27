#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gb.h"

int main(int argc, char **argv)
{
    GB gb;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s ROM_FILE\n", argv[0]);
        return 1;
    }

    if (!gb_init(&gb)) {
        fprintf(stderr, "Failed to initialize emulator\n");
        return 1;
    }

    if (!mmu_load_rom(gb.mmu, argv[1])) {
        gb_destroy(&gb);
        return 1;
    }

    gb.cartridge.loaded = true;
    gb.cartridge.rom_name = malloc(strlen(argv[1]) + 1u);
    if (gb.cartridge.rom_name != NULL) {
        strcpy(gb.cartridge.rom_name, argv[1]);
    }

    gb_destroy(&gb);
    return 0;
}


