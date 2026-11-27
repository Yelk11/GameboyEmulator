#include <stdio.h>

#include "gb.h"

int main(int argc, char **argv)
{
    MMU *mmu;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s ROM_FILE\n", argv[0]);
        return 1;
    }

    mmu = mmu_init();
    if (mmu == NULL) {
        fprintf(stderr, "Failed to allocate MMU\n");
        return 1;
    }

    if (!mmu_load_rom(mmu, argv[1])) {
        mmu_destroy(mmu);
        return 1;
    }

    mmu_destroy(mmu);
    return 0;
}


