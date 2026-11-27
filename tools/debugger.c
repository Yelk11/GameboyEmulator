#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "gb.h"

static int execute_instruction(GB *gb)
{
    uint32_t before = gb->cpu->cycles;
    if (!cpu_step(gb->cpu, gb->mmu)) return 1;
    debugger_trace_opcode(gb);
    {
        uint32_t cycles = gb->cpu->cycles - before;
        if (cycles == 0u) cycles = 4u;
        gb_tick(gb, cycles);
        interrupt_step(gb);
    }
    return 0;
}

static int execute_frame(GB *gb)
{
    uint32_t elapsed = 0u;
    uint64_t before = gb->total_t_cycles;
    while (elapsed < GB_T_CYCLES_PER_FRAME) {
        if (debugger_should_break(&gb->debugger, gb->cpu->pc)) {
            return 0;
        }
        if (execute_instruction(gb) != 0) return 1;
        elapsed = (uint32_t)(gb->total_t_cycles - before);
    }
    gb->frame_number++;
    return 0;
}

static bool parse_hex_address(const char *text, uint16_t *address)
{
    char *end = NULL;
    unsigned long value;

    if (text == NULL || text[0] == '\0' || address == NULL) return false;
    errno = 0;
    value = strtoul(text, &end, 16);
    if (errno != 0 || end == text || *end != '\0' || value > 0xFFFFu) {
        return false;
    }
    *address = (uint16_t)value;
    return true;
}

static bool parse_disassembly_count(const char *text, uint8_t *count)
{
    char *end = NULL;
    unsigned long value;

    if (text == NULL || text[0] == '\0' || count == NULL) return false;
    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        value == 0u || value > 64u) {
        return false;
    }
    *count = (uint8_t)value;
    return true;
}

static void print_disassembly(GB *gb, const char *address_arg,
                              const char *count_arg)
{
    uint16_t address;
    uint8_t count = 8u;
    uint8_t index;

    if (address_arg[0] == '\0') {
        address = gb->cpu->pc;
    } else if (!parse_hex_address(address_arg, &address)) {
        printf("Invalid address: %s\n", address_arg);
        return;
    }
    if (count_arg[0] != '\0' &&
        !parse_disassembly_count(count_arg, &count)) {
        printf("Invalid count (use 1-64): %s\n", count_arg);
        return;
    }

    for (index = 0u; index < count; index++) {
        char instruction[32];
        uint8_t length;
        uint8_t byte_index;

        if (!cpu_disassemble(gb->mmu, address, instruction,
                             sizeof(instruction), &length)) {
            printf("Cannot disassemble at %04X\n", address);
            return;
        }
        printf("%04X:", address);
        for (byte_index = 0u; byte_index < length; byte_index++) {
            printf(" %02X", mmu_read_byte(
                       gb->mmu, (uint16_t)(address + byte_index)));
        }
        for (; byte_index < 3u; byte_index++) printf("   ");
        printf("  %s\n", instruction);
        address = (uint16_t)(address + length);
    }
}

int main(int argc, char **argv)
{
    GB gb;
    char line[256];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s ROM_FILE\n", argv[0]);
        return 2;
    }
    if (!gb_init(&gb) || !mmu_load_rom(gb.mmu, argv[1])) {
        gb_destroy(&gb);
        return 2;
    }
    gb.debugger.paused = true;
    debugger_print_cpu(&gb);
    printf("Debugger commands: step, frame, run, breakpoint XXXX, cpu, ppu, mem XXXX, trace, quit\n");

    while (fgets(line, sizeof(line), stdin) != NULL) {
        char command[32];
        char argument[32];
        char count_argument[32];
        unsigned long value = 0u;
        command[0] = '\0';
        argument[0] = '\0';
        count_argument[0] = '\0';
        sscanf(line, "%31s %31s %31s", command, argument, count_argument);

        if (strcmp(command, "step") == 0) {
            if (execute_instruction(&gb) != 0) break;
            debugger_print_cpu(&gb);
        } else if (strcmp(command, "frame") == 0) {
            if (execute_frame(&gb) != 0) break;
            debugger_print_ppu(&gb);
        } else if (strcmp(command, "run") == 0) {
            for (;;) {
                if (debugger_should_break(&gb.debugger, gb.cpu->pc)) {
                    debugger_print_cpu(&gb);
                    break;
                }
                if (execute_instruction(&gb) != 0) {
                    gb_destroy(&gb);
                    return 3;
                }
            }
        } else if (strcmp(command, "breakpoint") == 0) {
            if (argument[0] == '\0') {
                printf("Breakpoint address required\n");
            } else {
                value = strtoul(argument, NULL, 16);
                if (!debugger_add_breakpoint(&gb.debugger, (uint16_t)value)) {
                    printf("Breakpoint limit reached or invalid debugger\n");
                } else {
                    printf("Breakpoint added at %04lX\n", value);
                }
            }
        } else if (strcmp(command, "cpu") == 0) {
            debugger_print_cpu(&gb);
        } else if (strcmp(command, "ppu") == 0) {
            debugger_print_ppu(&gb);
        } else if (strcmp(command, "mem") == 0) {
            if (argument[0] == '\0') {
                printf("Memory address required\n");
            } else {
                value = strtoul(argument, NULL, 16);
                debugger_print_memory(&gb, (uint16_t)value);
            }
        } else if (strcmp(command, "disasm") == 0) {
            print_disassembly(&gb, argument, count_argument);
        } else if (strcmp(command, "trace") == 0) {
            gb.debugger.opcode_trace = !gb.debugger.opcode_trace;
            printf("Opcode trace %s\n",
                   gb.debugger.opcode_trace ? "enabled" : "disabled");
        } else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            break;
        } else if (command[0] != '\0') {
            printf("Unknown command: %s\n", command);
        }
    }

    gb_destroy(&gb);
    return 0;
}
