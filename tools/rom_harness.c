#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gb.h"

static int contains_result(const char *output)
{
    if (strstr(output, "Passed") != NULL) return 1;
    if (strstr(output, "Failed") != NULL) return -1;
    if (strstr(output, "FAIL") != NULL) return -1;
    if (strstr(output, "PASS") != NULL) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    GB gb;
    char serial_output[4096];
    size_t serial_length = 0u;
    uint32_t frame_limit = 7200u;
    uint8_t previous_sc = 0u;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s ROM_FILE [FRAME_LIMIT]\n", argv[0]);
        return 2;
    }
    if (argc >= 3) {
        char *end = NULL;
        unsigned long parsed = strtoul(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0' || parsed == 0u) {
            fprintf(stderr, "Invalid frame limit: %s\n", argv[2]);
            return 2;
        }
        frame_limit = (uint32_t)parsed;
    }

    memset(serial_output, 0, sizeof(serial_output));
    if (!gb_init(&gb) || !mmu_load_rom(gb.mmu, argv[1])) {
        gb_destroy(&gb);
        return 2;
    }

    for (uint32_t frame = 0u; frame < frame_limit; frame++) {
        uint32_t elapsed = 0u;
        while (elapsed < GB_T_CYCLES_PER_FRAME) {
            uint32_t before = gb.cpu->cycles;
            if (!cpu_step(gb.cpu, gb.mmu)) {
                gb_destroy(&gb);
                fprintf(stderr, "Illegal opcode before frame %u\n", frame);
                return 3;
            }
            if (gb.cpu->cycles > before) {
                uint32_t cycles = gb.cpu->cycles - before;
                gb_tick(&gb, cycles);
                elapsed += cycles;
                interrupt_step(&gb);
            } else {
                gb_tick(&gb, 4u);
                elapsed += 4u;
            }
        }

        {
            uint8_t sc = gb.mmu->memory[0xFF02u];
            uint8_t sb = gb.mmu->memory[0xFF01u];
            if ((sc & 0x80u) != 0u && previous_sc == 0u) {
                previous_sc = sc;
            } else if ((sc & 0x80u) == 0u && previous_sc != 0u) {
                if (serial_length + 1u < sizeof(serial_output)) {
                    serial_output[serial_length++] = (char)sb;
                    serial_output[serial_length] = '\0';
                }
                previous_sc = 0u;

                int result = contains_result(serial_output);
                if (result != 0) {
                    gb_destroy(&gb);
                    printf("%s: %s\n", result > 0 ? "PASS" : "FAIL",
                           serial_output);
                    return result > 0 ? 0 : 1;
                }
            }
        }

        if ((frame % 100u) == 0u) {
            if (strlen(serial_output) > 0u) {
                fprintf(stderr, "Serial: %s\n", serial_output);
            }
        }
    }

    gb_destroy(&gb);
    fprintf(stderr, "HANG: frame limit reached (%u frames)\n", frame_limit);
    if (serial_length > 0u) fprintf(stderr, "Serial: %s\n", serial_output);
    return 124;
}
