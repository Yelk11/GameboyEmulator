#include "gb.h"

int main() {
    MMU* mmu = mmu_init();
    mmu_load_rom(mmu, "roms\\Legend of Zelda, The - Link's Awakening (USA, Europe) (Rev 2).gb");
    
    
    return 0;
}


