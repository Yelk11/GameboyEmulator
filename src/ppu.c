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
    ppu->stat_irq_line = false;
    ppu->mmu = NULL;
    memset(ppu->framebuffer, 0, sizeof(ppu->framebuffer));
    memset(ppu->sgb_framebuffer, 0, sizeof(ppu->sgb_framebuffer));
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
    case 0: return 0xFFFFFFFFu;
    case 1: return 0xFFAAAAAAu;
    case 2: return 0xFF555555u;
    default: return 0xFF000000u;
    }
}

static uint32_t cgb_palette_rgb(MMU *mmu, uint8_t palette,
                                uint8_t color, bool sprite)
{
    uint8_t *palettes = sprite ? mmu->obj_palette : mmu->bg_palette;
    uint16_t index = (uint16_t)(palette * 8u + color * 2u);
    uint16_t rgb = (uint16_t)(palettes[index] |
                              ((uint16_t)palettes[index + 1u] << 8));
    uint8_t r = (uint8_t)((rgb >> 0u) & 31u);
    uint8_t g = (uint8_t)((rgb >> 5u) & 31u);
    uint8_t b = (uint8_t)((rgb >> 10u) & 31u);
    r = (uint8_t)((r * 255u + 15u) / 31u);
    g = (uint8_t)((g * 255u + 15u) / 31u);
    b = (uint8_t)((b * 255u + 15u) / 31u);
    return 0xFF000000u | ((uint32_t)r << 16) |
           ((uint32_t)g << 8) | b;
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
        data_addr = (uint16_t)(0x9000 + (int16_t)((int8_t)tile) * 16);
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
    uint8_t bg_color_index[GB_FRAMEBUFFER_WIDTH] = {0u};
    bool window_drawn = false;
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
    for (x = 0u; x < GB_FRAMEBUFFER_WIDTH; x++) {
        uint8_t source_x = (uint8_t)((uint16_t)x + scx);
        uint8_t source_y = (uint8_t)((uint16_t)ly + scy);
        uint8_t color;
        uint8_t shade;

        if (ppu->mmu->cgb_mode) {
            uint8_t tile_x = (uint8_t)((source_x >> 3u) & 31u);
            uint8_t tile_y = (uint8_t)((source_y >> 3u) & 31u);
            uint16_t attr_addr = (uint16_t)(map_addr + tile_y * 32u + tile_x);
            uint8_t attr = ppu->mmu->vram[1][attr_addr - 0x8000u];
            uint16_t attr_tile_map = (uint16_t)(attr_addr - 0x9800u);
            uint16_t tile_addr = (uint16_t)(map_addr + tile_y * 32u + tile_x);
            uint8_t tile = ppu->mmu->vram[0][tile_addr - 0x8000u];
            uint8_t row = (uint8_t)(source_y & 7u);
            if (attr & 0x40u) row = (uint8_t)(7u - row);
            if ((lcdc & 0x10u) != 0u) {
                tile_addr = (uint16_t)(0x8000u + tile * 16u);
            } else {
                tile_addr = (uint16_t)(0x9000 + (int16_t)((int8_t)tile) * 16);
            }
            uint8_t bank = (attr & 0x08u) != 0u ? 1u : 0u;
            uint8_t low = ppu->mmu->vram[bank][tile_addr - 0x8000u + row * 2u];
            uint8_t high = ppu->mmu->vram[bank][tile_addr - 0x8000u + row * 2u + 1u];
            uint8_t bit = (attr & 0x20u) != 0u ? (x & 7u) : (uint8_t)(7u - (x & 7u));
            color = (uint8_t)(((high >> bit) & 1u) << 1u |
                              ((low >> bit) & 1u));
            bg_color_index[x] = color;
            ppu->framebuffer[ly * GB_FRAMEBUFFER_WIDTH + x] =
                cgb_palette_rgb(ppu->mmu, attr & 7u, color, false);
            (void)attr_tile_map;
            continue;
        }

        if ((lcdc & 0x01u) == 0u) {
            color = 0u;
        } else if ((lcdc & 0x20u) != 0u && ly >= wy && wx <= 166u &&
                   (int16_t)x >= (int16_t)wx - 7) {
            uint8_t window_x = (uint8_t)((int16_t)x - ((int16_t)wx - 7));
            uint16_t window_map = (lcdc & 0x40u) != 0u ? 0x9C00u : 0x9800u;
            color = background_tile_pixel(ppu, window_map,
                                          window_x, ppu->window_line);
            window_drawn = true;
        } else {
            color = background_tile_pixel(ppu, map_addr, source_x, source_y);
        }
        bg_color_index[x] = color;
        shade = shade_for_palette(ppu->mmu->memory[0xFF47u], color);
        ppu->framebuffer[ly * GB_FRAMEBUFFER_WIDTH + x] = shade_rgb(shade);
    }

    if ((lcdc & 0x02u) != 0u) {
        uint8_t sprites[10];
        uint8_t sprite_count = 0u;
        uint8_t sprite;
        uint8_t height = (lcdc & 0x04u) != 0u ? 16u : 8u;

        for (sprite = 0u; sprite < 40u && sprite_count < 10u; sprite++) {
            uint16_t oam = (uint16_t)(0xFE00u + sprite * 4u);
            int16_t sprite_y =
                (int16_t)mmu_read_byte(ppu->mmu, oam) - 16;

            if (ly < sprite_y || ly >= sprite_y + height) continue;
            sprites[sprite_count++] = sprite;
        }

        for (sprite = 0u; sprite < sprite_count; sprite++) {
            uint8_t other;
            for (other = (uint8_t)(sprite + 1u);
                 other < sprite_count; other++) {
                uint16_t sprite_oam =
                    (uint16_t)(0xFE00u + sprites[sprite] * 4u);
                uint16_t other_oam =
                    (uint16_t)(0xFE00u + sprites[other] * 4u);
                uint8_t sprite_x =
                    mmu_read_byte(ppu->mmu, (uint16_t)(sprite_oam + 1u));
                uint8_t other_x =
                    mmu_read_byte(ppu->mmu, (uint16_t)(other_oam + 1u));
                bool lower_priority = ppu->mmu->cgb_mode ?
                    sprites[sprite] < sprites[other] :
                    (sprite_x < other_x ||
                     (sprite_x == other_x &&
                      sprites[sprite] < sprites[other]));

                if (lower_priority) {
                    uint8_t swap = sprites[sprite];
                    sprites[sprite] = sprites[other];
                    sprites[other] = swap;
                }
            }
        }

        for (sprite = 0u; sprite < sprite_count; sprite++) {
            uint16_t oam = (uint16_t)(0xFE00u + sprites[sprite] * 4u);
            int16_t sprite_y = (int16_t)mmu_read_byte(ppu->mmu, oam) - 16;
            int16_t sprite_x =
                (int16_t)mmu_read_byte(ppu->mmu, (uint16_t)(oam + 1u)) - 8;
            uint8_t tile =
                mmu_read_byte(ppu->mmu, (uint16_t)(oam + 2u));
            uint8_t attributes =
                mmu_read_byte(ppu->mmu, (uint16_t)(oam + 3u));
            uint8_t row;
            uint8_t tile_low;
            uint8_t tile_high;

            row = (uint8_t)(ly - sprite_y);
            if ((attributes & 0x40u) != 0u) row = (uint8_t)(height - 1u - row);
            if (height == 16u) {
                tile &= 0xFEu;
                if (row >= 8u) { tile++; row = (uint8_t)(row - 8u); }
            }
            if (ppu->mmu->cgb_mode && (attributes & 0x08u) != 0u) {
                uint16_t offset = (uint16_t)(tile * 16u + row * 2u);
                tile_low = ppu->mmu->vram[1][offset];
                tile_high = ppu->mmu->vram[1][offset + 1u];
            } else {
                tile_low = mmu_read_byte(ppu->mmu, (uint16_t)(0x8000u + tile * 16u + row * 2u));
                tile_high = mmu_read_byte(ppu->mmu, (uint16_t)(0x8000u + tile * 16u + row * 2u + 1u));
            }

            for (x = 0u; x < 8u; x++) {
                int16_t screen_x = sprite_x + x;
                uint8_t bit = (attributes & 0x20u) != 0u ? x : (uint8_t)(7u - x);
                uint8_t color = (uint8_t)(((tile_high >> bit) & 1u) << 1u |
                                          ((tile_low >> bit) & 1u));
                if (screen_x < 0 ||
                    (uint32_t)screen_x >= GB_FRAMEBUFFER_WIDTH ||
                    color == 0u) continue;
                if ((attributes & 0x80u) != 0u &&
                    bg_color_index[screen_x] != 0u) continue;
                {
                    uint8_t palette = (attributes & 0x10u) != 0u ?
                                      ppu->mmu->memory[0xFF49u] : ppu->mmu->memory[0xFF48u];
                    if (ppu->mmu->cgb_mode) {
                        ppu->framebuffer[ly * GB_FRAMEBUFFER_WIDTH + screen_x] =
                            cgb_palette_rgb(ppu->mmu, attributes & 7u, color, true);
                    } else {
                        ppu->framebuffer[ly * GB_FRAMEBUFFER_WIDTH + screen_x] =
                            shade_rgb(shade_for_palette(palette, color));
                    }
                }
            }
        }
    }
    if (window_drawn) ppu->window_line++;
}

void ppu_compose_sgb(PPU *ppu)
{
    uint32_t y;
    uint32_t x;
    if (ppu == NULL || ppu->mmu == NULL ||
        !ppu->mmu->sgb.enabled) return;

    for (y = 0u; y < SGB_BORDER_HEIGHT; y++) {
        for (x = 0u; x < SGB_BORDER_WIDTH; x++) {
            ppu->sgb_framebuffer[y * SGB_BORDER_WIDTH + x] =
                ppu->mmu->sgb.border[y * SGB_BORDER_WIDTH + x];
        }
    }
    for (y = 0u; y < GB_FRAMEBUFFER_HEIGHT; y++) {
        for (x = 0u; x < GB_FRAMEBUFFER_WIDTH; x++) {
            ppu->sgb_framebuffer[(y + 40u) * SGB_BORDER_WIDTH +
                                 (x + 48u)] =
                ppu->framebuffer[y * GB_FRAMEBUFFER_WIDTH + x];
        }
    }
}

static void ppu_update_stat(PPU *ppu, bool lcd_enabled)
{
    uint8_t stat = ppu->mmu->memory[0xFF41u];
    bool coincidence = ppu->ly == ppu->mmu->memory[0xFF45u];
    bool stat_irq;

    stat = (uint8_t)((stat & 0xF8u) | (uint8_t)ppu->mode);
    if (coincidence) stat |= 0x04u;
    else stat &= (uint8_t)~0x04u;
    ppu->mmu->memory[0xFF41u] = stat;
    ppu->mmu->memory[0xFF44u] = ppu->ly;
    ppu->mmu->ppu_mode = (uint8_t)ppu->mode;

    stat_irq = lcd_enabled &&
        (((ppu->mode == PPU_MODE_HBLANK) && (stat & 0x08u) != 0u) ||
         ((ppu->mode == PPU_MODE_VBLANK) && (stat & 0x10u) != 0u) ||
         ((ppu->mode == PPU_MODE_OAM) && (stat & 0x20u) != 0u) ||
         (coincidence && (stat & 0x40u) != 0u));
    if (stat_irq && !ppu->stat_irq_line) {
        ppu->mmu->memory[0xFF0Fu] |= 0x02u;
    }
    ppu->stat_irq_line = stat_irq;
}

void ppu_update(PPU *ppu, uint32_t t_cycles)
{
    uint32_t remaining;

    if (ppu == NULL || ppu->mmu == NULL) {
        return;
    }

    if ((ppu->mmu->memory[0xFF40u] & 0x80u) == 0u) {
        ppu->mode = PPU_MODE_HBLANK;
        ppu->ly = 0u;
        ppu->line_dot = 0u;
        ppu->frame_cycle = 0u;
        ppu->window_line = 0u;
        ppu->stat_irq_line = false;
        ppu_update_stat(ppu, false);
        return;
    }

    remaining = t_cycles;
    while (remaining > 0u) {
        uint32_t step = remaining;
        uint32_t until_line_end = GB_T_CYCLES_PER_SCANLINE - ppu->line_dot;
        uint32_t until_mode_change;

        if (ppu->ly >= 144u) {
            until_mode_change = until_line_end;
        } else if (ppu->line_dot < 80u) {
            until_mode_change = 80u - ppu->line_dot;
        } else if (ppu->line_dot < 252u) {
            until_mode_change = 252u - ppu->line_dot;
        } else {
            until_mode_change = until_line_end;
        }
        if (step > until_mode_change) step = until_mode_change;

        if (step > until_line_end) {
            step = until_line_end;
        }

        ppu->line_dot += (uint16_t)step;
        ppu->frame_cycle = (ppu->frame_cycle + step) % GB_T_CYCLES_PER_FRAME;
        remaining -= step;

        if (ppu->line_dot == GB_T_CYCLES_PER_SCANLINE) {
            if (ppu->ly < 144u) {
                ppu_render_line(ppu);
                ppu_compose_sgb(ppu);
            }
            ppu->line_dot = 0u;
            ppu->ly = (uint8_t)((ppu->ly + 1u) % GB_SCANLINE_COUNT);
            if (ppu->ly == 0u) ppu->window_line = 0u;
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
        ppu_update_stat(ppu, true);
        if (ppu->ly == 144u && ppu->line_dot == 0u) {
            ppu->mmu->memory[0xFF0Fu] |= 0x01u;
        }
    }
}