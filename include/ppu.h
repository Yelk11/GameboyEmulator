#ifndef PPU_H
#define PPU_H

#include <stdbool.h>
#include <stdint.h>
#include "mmu.h"

typedef enum PPU_MODE {
    PPU_MODE_HBLANK = 0,
    PPU_MODE_VBLANK = 1,
    PPU_MODE_OAM = 2,
    PPU_MODE_TRANSFER = 3
} PPU_MODE;

typedef struct PPU {
    PPU_MODE mode;
    uint8_t ly;
    uint16_t line_dot;
    uint32_t frame_cycle;
    uint8_t window_line;
    bool stat_irq_line;
    uint32_t framebuffer[160u * 144u];
    uint32_t sgb_framebuffer[256u * 224u];
    MMU *mmu;
} PPU;

void ppu_init(PPU *ppu);
void ppu_update(PPU *ppu, uint32_t t_cycles);
void ppu_connect(PPU *ppu, MMU *mmu);
void ppu_compose_sgb(PPU *ppu);

#endif