#include "gb.h"
#include "ppu.h"
#include <string.h>

void ppu_init(PPU *ppu)
{
    if (ppu == NULL) {
        return;
    }

    ppu->mode = PPU_MODE_OAM;
    ppu->ly = 0u;
    ppu->line_dot = 0u;
    ppu->frame_cycle = 0u;
    ppu->mmu = NULL;
    memset(ppu->framebuffer, 0, sizeof(ppu->framebuffer));
}

void ppu_connect(PPU *ppu, MMU *mmu)
{
    if (ppu != NULL) ppu->mmu = mmu;
}

static uint8_t shade_for_palette(uint8_t palette, uint8_t color)
{
    return (uint8_t)((palette >> (color * 2u)) & 0x03u);
}

static uint32_t shade_rgb(uint8_t shade)
{
    switch (shade) {
    case 0: return 0xFFE0F8D0u;
    case 1: return 0xFF88C070u;
    case 2: return 0xFF346856u;
    default: return 0xFF081820u;
    }
}

static uint8_t background_tile_pixel(PPU *ppu, uint16_t map_addr,
                                     uint8_t x, uint8_t y)
{
    uint16_t tile_map_x = (uint16_t)((x >> 3u) & 31u);
    uint16_t tile_map_y = (uint16_t)((y >> 3u) & 31u);
    uint16_t tile_addr = (uint16_t)(map_addr + tile_map_y * 32u + tile_map_x);
    uint8_t tile = mmu_read_byte(ppu->mmu, tile_addr);
    uint16_t data_addr;
    uint8_t row = (uint8_t)(y & 7u);
    uint8_t bit = (uint8_t)(7u - (x & 7u));
    uint8_t low;
    uint8_t high;

    if ((ppu->mmu->memory[0xFF40u] & 0x10u) != 0u) {
        data_addr = (uint16_t)(0x8000u + (uint16_t)tile * 16u);
    } else {
        data_addr = (uint16_t)(0x9000u + (int16_t)((int8_t)tile) * 16);
    }
    low = mmu_read_byte(ppu->mmu, (uint16_t)(data_addr + row * 2u));
    high = mmu_read_byte(ppu->mmu, (uint16_t)(data_addr + row * 2u + 1u));
    return (uint8_t)(((high >> bit) & 1u) << 1u | ((low >> bit) & 1u));
}

static void ppu_render_line(PPU *ppu)
{
    uint8_t lcdc;
    uint8_t ly;
    uint8_t scx;
    uint8_t scy;
    uint8_t wx;
    uint8_t wy;
    uint16_t map_addr;
    uint8_t x;

    if (ppu == NULL || ppu->mmu == NULL) return;
    lcdc = ppu->mmu->memory[0xFF40u];
    ly = ppu->ly;
    scx = ppu->mmu->memory[0xFF43u];
    scy = ppu->mmu->memory[0xFF42u];
    wx = ppu->mmu->memory[0xFF4Bu];
    wy = ppu->mmu->memory[0xFF4Au];
    map_addr = (lcdc & 0x08u) != 0u ? 0x9C00u : 0x9800u;

    if (ly >= 144u) return;
    if ((lcdc & 0x01u) == 0u) {
        for (x = 0u; x < GB_FRAMEBUFFER_WIDTH; x++) {
            ppu->framebuffer[ly * GB_FRAMEBUFFER_WIDTH + x] = shade_rgb(3u);
        }
        return;
    }

    for (x = 0u; x < GB_FRAMEBUFFER_WIDTH; x++) {
        uint8_t source_x = (uint8_t)((uint16_t)x + scx);
        uint8_t source_y = (uint8_t)((uint16_t)ly + scy);
        uint8_t color;
        uint8_t shade;

        if ((lcdc & 0x20u) != 0u && ly >= wy && x + 7u >= wx) {
            uint8_t window_x = (uint8_t)(x - (wx > 7u ? wx - 7u : 0u));
            uint16_t window_map = (lcdc & 0x40u) != 0u ? 0x9C00u : 0x9800u;
            color = background_tile_pixel(ppu, window_map,
                                          window_x, (uint8_t)(ly - wy));
        } else {
            color = background_tile_pixel(ppu, map_addr, source_x, source_y);
        }
        shade = shade_for_palette(ppu->mmu->memory[0xFF47u], color);
        ppu->framebuffer[ly * GB_FRAMEBUFFER_WIDTH + x] = shade_rgb(shade);
    }

    if ((lcdc & 0x02u) != 0u) {
        uint8_t sprite_count;
        uint8_t sprite;
        uint8_t height = (lcdc & 0x04u) != 0u ? 16u : 8u;

        for (sprite = 0u, sprite_count = 0u; sprite < 40u && sprite_count < 10u; sprite++) {
            uint16_t oam = (uint16_t)(0xFE00u + sprite * 4u);
            int16_t sprite_y = (int16_t)mmu_read_byte(ppu->mmu, oam) - 16;
            int16_t sprite_x = (int16_t)mmu_read_byte(ppu->mmu, (uint16_t)(oam + 1u)) - 8;
            uint8_t tile = mmu_read_byte(ppu->mmu, (uint16_t)(oam + 2u));
            uint8_t attributes = mmu_read_byte(ppu->mmu, (uint16_t)(oam + 3u));
            uint8_t row;
            uint8_t tile_low;
            uint8_t tile_high;

            if (ly < sprite_y || ly >= sprite_y + height) continue;
            sprite_count++;
            row = (uint8_t)(ly - sprite_y);
            if ((attributes & 0x40u) != 0u) row = (uint8_t)(height - 1u - row);
            if (height == 16u) {
                tile &= 0xFEu;
                if (row >= 8u) { tile++; row = (uint8_t)(row - 8u); }
            }
            tile_low = mmu_read_byte(ppu->mmu, (uint16_t)(0x8000u + tile * 16u + row * 2u));
            tile_high = mmu_read_byte(ppu->mmu, (uint16_t)(0x8000u + tile * 16u + row * 2u + 1u));

            for (x = 0u; x < 8u; x++) {
                int16_t screen_x = sprite_x + x;
                uint8_t bit = (attributes & 0x20u) != 0u ? x : (uint8_t)(7u - x);
                uint8_t color = (uint8_t)(((tile_high >> bit) & 1u) << 1u |
                                          ((tile_low >> bit) & 1u));
                if (screen_x < 0 ||
                    (uint32_t)screen_x >= GB_FRAMEBUFFER_WIDTH ||
                    color == 0u) continue;
                if ((attributes & 0x80u) != 0u &&
                    ppu->framebuffer[ly * GB_FRAMEBUFFER_WIDTH + screen_x] !=
                    shade_rgb(shade_for_palette(ppu->mmu->memory[0xFF47u], 0u))) continue;
                {
                    uint8_t palette = (attributes & 0x10u) != 0u ?
                                      ppu->mmu->memory[0xFF49u] : ppu->mmu->memory[0xFF48u];
                    ppu->framebuffer[ly * GB_FRAMEBUFFER_WIDTH + screen_x] =
                        shade_rgb(shade_for_palette(palette, color));
                }
            }
        }
    }
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
            if (ppu->ly < 144u) ppu_render_line(ppu);
            ppu->line_dot = 0u;
            ppu->ly = (uint8_t)((ppu->ly + 1u) % GB_SCANLINE_COUNT);
            if (ppu->mmu != NULL) ppu->mmu->memory[0xFF44u] = ppu->ly;
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
        if (ppu->mmu != NULL) {
            uint8_t stat = ppu->mmu->memory[0xFF41u];
            stat = (uint8_t)((stat & 0xF8u) | (uint8_t)ppu->mode);
            if (ppu->ly == ppu->mmu->memory[0xFF45u]) stat |= 0x04u;
            else stat &= (uint8_t)~0x04u;
            ppu->mmu->memory[0xFF41u] = stat;
            ppu->mmu->memory[0xFF44u] = ppu->ly;
            ppu->mmu->ppu_mode = (uint8_t)ppu->mode;
        }
        if (ppu->ly == 144u && ppu->line_dot == 0u && ppu->mmu != NULL) {
            ppu->mmu->memory[0xFF0Fu] |= 0x01u;
        }
    }
}