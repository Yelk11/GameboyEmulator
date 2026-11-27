#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "gb.h"

static void apply_key(Joypad *joypad, SDL_Keycode key, bool pressed)
{
    switch (key) {
    case SDLK_RIGHT: joypad_press(joypad, 4u, pressed); break;
    case SDLK_LEFT: joypad_press(joypad, 5u, pressed); break;
    case SDLK_UP: joypad_press(joypad, 6u, pressed); break;
    case SDLK_DOWN: joypad_press(joypad, 7u, pressed); break;
    case SDLK_z: joypad_press(joypad, 0u, pressed); break;
    case SDLK_x: joypad_press(joypad, 1u, pressed); break;
    case SDLK_RETURN: joypad_press(joypad, 3u, pressed); break;
    case SDLK_RSHIFT:
    case SDLK_LSHIFT: joypad_press(joypad, 2u, pressed); break;
    default: break;
    }
}

static int run_frame(GB *gb)
{
    uint32_t elapsed = 0u;

    if (gb->debugger.paused &&
        !gb->debugger.step_instruction &&
        !gb->debugger.step_frame) {
        return 0;
    }

    while (elapsed < GB_T_CYCLES_PER_FRAME) {
        uint32_t before = gb->cpu->cycles;
        uint32_t interrupt_cycles = 0u;

        if (debugger_should_break(&gb->debugger, gb->cpu->pc)) {
            gb->debugger.paused = true;
            return 0;
        }
        debugger_trace_opcode(gb);
        if (!cpu_step(gb->cpu, gb->mmu)) {
            return 1;
        }

        {
            uint32_t instruction_cycles = gb->cpu->cycles - before;
            if (instruction_cycles == 0u) instruction_cycles = 4u;
            gb_tick(gb, instruction_cycles);
            elapsed += instruction_cycles;
            interrupt_cycles = interrupt_step(gb);
            if (interrupt_cycles != 0u) {
                gb_tick(gb, interrupt_cycles);
                elapsed += interrupt_cycles;
            }
        }

        if (gb->debugger.step_instruction) {
            gb->debugger.step_instruction = false;
            gb->debugger.paused = true;
            debugger_print_cpu(gb);
            return 0;
        }
        if (gb->debugger.step_frame) {
            gb->debugger.step_frame = false;
            gb->debugger.paused = true;
            debugger_print_ppu(gb);
            gb->frame_number++;
            return 0;
        }
    }
    gb->frame_number++;
    return 0;
}

int main(int argc, char **argv)
{
    GB gb;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    bool running = true;
    uint64_t frame_start;
    int render_width;
    int render_height;

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

    {
        uint32_t width = mmu_is_sgb(gb.mmu) ?
                         SGB_BORDER_WIDTH : GB_FRAMEBUFFER_WIDTH;
        uint32_t height = mmu_is_sgb(gb.mmu) ?
                          SGB_BORDER_HEIGHT : GB_FRAMEBUFFER_HEIGHT;
        (void)width;
        (void)height;
    }

    gb.cartridge.loaded = true;
    gb.cartridge.rom_name = malloc(strlen(argv[1]) + 1u);
    if (gb.cartridge.rom_name != NULL) {
        strcpy(gb.cartridge.rom_name, argv[1]);
    }

    if (mmu_is_sgb(gb.mmu)) {
        render_width = (int)SGB_BORDER_WIDTH;
        render_height = (int)SGB_BORDER_HEIGHT;
    } else {
        render_width = (int)GB_FRAMEBUFFER_WIDTH;
        render_height = (int)GB_FRAMEBUFFER_HEIGHT;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        gb_destroy(&gb);
        return 1;
    }

    window = SDL_CreateWindow("Game Boy Emulator",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              render_width * 4, render_height * 4,
                              SDL_WINDOW_RESIZABLE);
    renderer = window != NULL ? SDL_CreateRenderer(window, -1,
                                                   SDL_RENDERER_ACCELERATED |
                                                   SDL_RENDERER_PRESENTVSYNC) : NULL;
    texture = renderer != NULL ?
              SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                render_width, render_height) : NULL;

    if (window == NULL || renderer == NULL || texture == NULL) {
        fprintf(stderr, "SDL display initialization failed: %s\n", SDL_GetError());
        if (texture != NULL) SDL_DestroyTexture(texture);
        if (renderer != NULL) SDL_DestroyRenderer(renderer);
        if (window != NULL) SDL_DestroyWindow(window);
        SDL_Quit();
        gb_destroy(&gb);
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    while (running) {
        SDL_Event event;
        frame_start = SDL_GetTicks64();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_F9) {
                    gb.debugger.paused = !gb.debugger.paused;
                    if (gb.debugger.paused) debugger_print_cpu(&gb);
                } else if (event.key.keysym.sym == SDLK_F10) {
                    gb.debugger.paused = false;
                    gb.debugger.step_instruction = true;
                } else if (event.key.keysym.sym == SDLK_F11) {
                    gb.debugger.paused = false;
                    gb.debugger.step_frame = true;
                } else if (event.key.keysym.sym == SDLK_F5) {
                    if (gb_save_state(&gb, 1u)) {
                        fprintf(stdout, "Saved state slot 1\n");
                    } else {
                        fprintf(stderr, "Failed to save state slot 1\n");
                    }
                } else if (event.key.keysym.sym == SDLK_F6) {
                    if (!gb_load_state(&gb, 1u)) {
                        fprintf(stderr, "Failed to load state slot 1\n");
                    } else {
                        fprintf(stdout, "Loaded state slot 1\n");
                    }
                } else {
                    apply_key(&gb.joypad, event.key.keysym.sym, true);
                }
            } else if (event.type == SDL_KEYUP) {
                apply_key(&gb.joypad, event.key.keysym.sym, false);
            }
        }

        if (!running) break;
        if (run_frame(&gb) != 0) {
            fprintf(stderr, "Emulation stopped on an illegal/unimplemented opcode\n");
            break;
        }

        if (mmu_is_sgb(gb.mmu)) {
            SDL_UpdateTexture(texture, NULL, gb.ppu->sgb_framebuffer,
                              SGB_BORDER_WIDTH * sizeof(uint32_t));
        } else {
            SDL_UpdateTexture(texture, NULL, gb.ppu->framebuffer,
                              GB_FRAMEBUFFER_WIDTH * sizeof(uint32_t));
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        {
            uint64_t elapsed = SDL_GetTicks64() - frame_start;
            if (elapsed < 16u) SDL_Delay((uint32_t)(16u - elapsed));
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    gb_destroy(&gb);
    return 0;
}


