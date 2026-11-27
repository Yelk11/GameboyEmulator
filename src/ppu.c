#include "gb.h"
#include "gb.h"
#include "ppu.h"

void ppu_init(PPU *ppu)
{
    if (ppu == NULL) {
        return;
    }

    ppu->mode = PPU_MODE_OAM;
    ppu->ly = 0u;
    ppu->line_dot = 0u;
    ppu->frame_cycle = 0u;
}

void ppu_update(PPU *ppu, uint32_t t_cycles)
{
    uint32_t remaining;

    if (ppu == NULL) {
        return;
    }

    remaining = t_cycles;
    while (remaining > 0u) {
        uint32_t step = remaining;
        uint32_t until_line_end = GB_T_CYCLES_PER_SCANLINE - ppu->line_dot;

        if (step > until_line_end) {
            step = until_line_end;
        }

        ppu->line_dot += (uint16_t)step;
        ppu->frame_cycle = (ppu->frame_cycle + step) % GB_T_CYCLES_PER_FRAME;
        remaining -= step;

        if (ppu->line_dot == GB_T_CYCLES_PER_SCANLINE) {
            ppu->line_dot = 0u;
            ppu->ly = (uint8_t)((ppu->ly + 1u) % GB_SCANLINE_COUNT);
        }

        if (ppu->ly >= 144u) {
            ppu->mode = PPU_MODE_VBLANK;
        } else if (ppu->line_dot < 80u) {
            ppu->mode = PPU_MODE_OAM;
        } else if (ppu->line_dot < 252u) {
            ppu->mode = PPU_MODE_TRANSFER;
        } else {
            ppu->mode = PPU_MODE_HBLANK;
        }
    }
}