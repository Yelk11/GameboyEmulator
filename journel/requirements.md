# Game Boy Emulator Implementation Requirements

## 1. Project Objective

Build a playable, mostly cycle-accurate Game Boy emulator in C for macOS and Windows.
The primary target is **DMG-01**, with **Game Boy Color and Super Game Boy** compatibility as later compatibility milestones. Use **SDL2** for the display and host input.

Initial core scope:

- Load cartridges from a command-line ROM filename.
- Emulate the CPU, MMU, PPU, timers, interrupts, joypad, cartridge controllers, and battery saves.
- Target near cycle-accurate PPU and CPU behavior.
- Include a debugger.
- Pass Tetris and Dr. Mario.
- Pass the chosen commercial game, The Legend of Zelda (the current hard-coded ROM is Link's Awakening), as a playable title.
- Add audio only after the video and gameplay core is complete.

This document is the implementation backlog. Complete phases in order unless a phase explicitly permits an independent task.

## 2. Confirmed Decisions

| Area | Decision |
| --- | --- |
| Hardware | DMG first; add GBC and SGB compatibility later |
| Frontend | SDL2 |
| Boot behavior | Skip boot ROM and initialize post-boot state |
| ROM loading | Command-line filename argument |
| Host platforms | macOS and Windows |
| Timing | Near cycle-accurate PPU and CPU timing |
| Save storage | Battery save files next to the ROM |
| Controllers | No-MBC/ROM-only, MBC1, MBC2, MBC3, MBC5, MMM01, MBC6/MBC7 |
| Save features | Battery RAM, RTC, save states |
| I/O | Joypad, DMA, timers, serial link |
| PPU | Background, window, sprites, LCD modes, VBlank/STAT |
| Debug | Debugger required |
| Audio | Not in the first milestone; add later |

## 3. Acceptance Criteria

The project is complete when all of the following are true:

1. `make test` passes on macOS and Windows.
2. A ROM can be launched with a command such as:
   ```text
   gameboy path/to/game.gb
   ```
3. The emulator opens an SDL2 window, renders 160x144 pixels, accepts keyboard input, and runs at approximately 59.7 frames/second.
4. Tetris and Dr. Mario boot, display correctly, accept input, and remain playable.
5. The selected Zelda title boots and is playable through normal menu and gameplay interactions.
6. DMG save data is persistent in a file beside the ROM.
7. MBC1, MBC2, MBC3, MBC5, MMM01, MBC6, and MBC7 cartridges can load their ROM and RAM banking behavior as required.
8. CPU, PPU, timer, interrupt, and DMA behavior is tested independently, not only by playing full games.
9. Save states restore CPU, memory, cartridge, PPU, timer, interrupt, and controller state.
10. GBC and SGB behavior is implemented only after the DMG acceptance set passes.

## 4. Implementation Order

### Phase 1: Stabilize the Build and Test Harness

**Goal:** Establish a reliable build before adding emulator behavior.

1. Repair the Unity test runner.
   - Remove or correctly exclude `tests/test_rom.c` until it is a valid test.
   - Add `tests/test_rom.c` to the runner only after adding a complete MMU test fixture.
   - Register every real test in `tests/test_runner.c`.
   - Ensure no test-only source contains broken or unregistered functions.
2. Replace C99-only assumptions that currently fail under the strict C standard.
   - Define named 8-bit register fields or use standard-named unions accepted by C99.
   - Confirm the AF, BC, DE, and HL register pairs behave correctly on little- and big-endian hosts.
3. Add allocation and file-loading failure handling.
   - Never dereference a NULL MMU.
   - Return explicit errors for missing or unreadable ROM files.
   - Validate file size before copying.
   - Prevent reads past ROM, cartridge, or RAM boundaries.
4. Make the build cross-platform.
   - Replace Windows-style-only assumptions in launch paths.
   - Use a portable ROM argument rather than a hard-coded path.
   - Add SDL2 discovery rules for macOS and Windows.
   - Define separate application and test targets.
5. Add compiler and sanitizer targets.
   - Keep `-Wall -Wextra -pedantic`.
   - Add AddressSanitizer/UndefinedBehaviorSanitizer targets for local debugging.

**Exit gate:** `make`, `make test`, and `make clean` work on macOS and Windows.

### Phase 2: Define the Emulator State and Clock Model

**Status:** Complete

**Goal:** Create one consistent machine-state model and timing unit.

1. Replace the incomplete GB structure with a usable emulator context.
   - Store CPU, MMU, PPU, timer, cartridge, joypad, interrupt, and option state.
   - Keep ownership clear: the emulator context owns subsystems; each subsystem exposes small update functions.
2. Define T-cycle timing consistently throughout the project.
   - Use DMG 4.194304 MHz as the nominal clock.
   - Treat T-cycles as the implementation unit.
   - Advance one instruction, PPU, timer, DMA, and subsystem update using exact T-cycle counts.
   - Preserve alternate/branch timing from the opcode metadata.
3. Define frame and line timing.
   - A frame is 70224 T-cycles.
   - A scanline is 456 T-cycles.
   - Visible lines are 0–143; VBlank lines are 144–153.
4. Establish post-boot CPU state.
   - Initialize PC to `0x0100`, SP to `0xFFFE`, and the initial register/flag values defined for DMG.
   - Initialize cartridge ROM selection and hardware registers.
   - Document exactly how GBC differs from DMG initialization.

**Exit gate:** Every subsystem update can accept a T-cycle delta and tests can advance the machine deterministically.

### Phase 3: Implement the Memory Bus and Cartridge System

**Status:** Complete

**Goal:** Make every 16-bit address map to the correct subsystem.

1. Redesign MMU/Bus memory access.
   - Replace the direct 64-KiB array-only implementation with region-aware reads and writes.
   - Implement ROM, VRAM, external RAM, WRAM, echo RAM, OAM, prohibited addresses, I/O, HRAM, IE, and IF behavior.
   - Reject or model invalid accesses explicitly.
   - Route all cartridge writes through the cartridge controller.
2. Implement ROM loading and header parsing.
   - Read the full cartridge file.
   - Validate minimum/maximum size and truncate or report unsupported sizes clearly.
   - Parse cartridge type, ROM size, RAM size, destination code, and CGB/SGB flags.
   - Detect ROM capacity and external RAM capacity from the cartridge header.
3. Implement cartridge controllers in this order:
   - No-MBC/ROM-only cartridges.
   - MBC1.
   - MBC3 with optional RTC.
   - MBC5.
   - MBC2.
   - MMM01.
   - MBC6 and MBC7 only after the earlier controllers and core pass.
4. Implement cartridge ROM/RAM paging.
   - Bank 0, switchable ROM, external RAM enable/disable, and RAM banking.
   - Implement MBC1 mode and bank-range edge behavior.
   - Implement MBC2 512x4-bit RAM behavior.
   - Implement MBC3 RTC registers and latch behavior.
   - Implement MBC5 9-bit ROM and 4-bit RAM banking.
5. Implement battery saves.
   - Derive the save filename next to the ROM.
   - Load external RAM before execution.
   - Save on clean shutdown and at safe intervals.
   - Preserve RTC state for MBC3 if RTC is present.

**Exit gate:** Unit tests cover every memory region and each implemented MBC bank behavior.

### Phase 4: Implement the LR35902 CPU

**Status:** Complete

**Goal:** Implement all standard and CB-prefixed instructions with correct flags and timing.

1. Replace the opcode metadata's unimplemented entries with real handlers.
   - Implement the 256 non-CB opcodes.
   - Implement all 256 CB-prefixed opcodes.
   - Return the exact taken/not-taken cycle count for conditional instructions.
2. Implement 8-bit loads and register transfers.
   - All LD register, immediate, memory, and indirect combinations.
   - LDH and C-register address forms.
   - LDI/LDD behavior.
3. Implement 16-bit loads and stack operations.
   - LD pair immediate, pair to memory, memory to pair, SP and HL operations.
   - PUSH, POP, CALL, RET, RETI, RST.
   - Signed r8 handling and stack alignment.
4. Implement ALU operations.
   - ADD, ADC, SUB, SBC, AND, XOR, OR, CP.
   - INC, DEC, CPL, DAA, CCF, SCF.
   - Correct Z, N, H, and C flag behavior.
5. Implement 16-bit arithmetic and jumps.
   - ADD HL, add/subtract SP forms, INC/DEC pair and SP.
   - JP, JR, conditional taken/not-taken behavior.
   - HALT and STOP behavior.
6. Implement rotate, shift, bit, set, and reset instructions.
   - RLC, RRC, RL, RR and CB variants.
   - SLA, SRA, SRL, SWAP.
   - BIT, RES, SET.
7. Implement interrupt and CPU control timing.
   - IME enable/disable, EI delay, DI.
   - HALT/halt-bug behavior.
   - STOP behavior initially required for DMG compatibility.
   - Interrupt servicing and RETI timing.
8. Replace illegal-opcode behavior with diagnostic state.
   - Print opcode, PC, register state, and stop or pause according to debugger settings.
   - Do not silently continue through an unimplemented instruction.

**Exit gate:** CPU is the first fully passing subsystem; opcode unit tests and external CPU test ROMs pass.

### Phase 5: Implement Timers, Interrupts, DMA, and Joypad

**Status:** Complete

**Goal:** Complete the hardware events that coordinate CPU execution.

1. Implement the interrupt controller.
   - IF at `0xFF0F`, IE at `0xFFFF`.
   - VBlank, STAT, Timer, Serial, and Joypad sources.
   - Priority, vectoring, stack push, and IME behavior.
2. Implement timers.
   - DIV, TIMA, TMA, TAC.
   - Edge-triggered timer increments and TIMA overflow timing.
   - DIV reset behavior and write timing.
3. Implement P1/JOYP.
   - Select action and D-pad banks.
   - Active-low key reporting.
   - Joypad interrupt for the correct press transition.
4. Implement OAM DMA.
   - Start and cycle timing from `0xFF46`.
   - Bus restrictions and copying behavior.
   - Add the GBC-specific DMA variant later.
5. Implement serial I/O.
   - SC, SB, transfer timing, internal clock behavior.
   - External link devices do not need networking for the initial milestone.
   - Safely represent an idle/disconnected serial link.

**Exit gate:** Timer, interrupt, DMA, joypad, and serial timing tests pass.

### Phase 6: Implement the DMG PPU

**Status:** Complete

**Goal:** Render a 160x144 frame with correct timing and layering.

1. Implement PPU state and LCD timing.
   - HBlank, OAM Search, Pixel Transfer, and VBlank modes.
   - Increment LY and wrap 153 to 0.
   - Implement LY=LYC compare and STAT interrupt sources.
2. Implement LCD control and status registers.
   - LCDC, STAT, SCY, SCX, LY, LYC, BGP, OBP0, OBP1, WY, WX.
   - LCD disable/enable behavior.
3. Implement tile and background rendering.
   - 2bpp tile decoding.
   - 32x32 tile maps at `0x9800` and `0x9C00`.
   - Signed and unsigned tile-addressing modes.
   - SCX/SCY wrapping.
4. Implement the window.
   - WX/WY positioning and window enable rules.
   - Window line counters and tile-map/window interaction.
5. Implement sprites.
   - 40 OAM entries, 10 sprites per scanline maximum.
   - 8x8 and 8x16 sprite sizes.
   - X flip, Y flip, palette, priority, and per-pixel transparency.
6. Implement VRAM/OAM bus restrictions.
   - Read/write restrictions in active PPU modes.
   - DMG behavior for blocked memory accesses.
7. Implement framebuffer presentation.
   - Render an internal 160x144 BGRA/RGBA buffer.
   - Copy it to an SDL2 texture once per frame.
   - Scale without changing the logical framebuffer size.
8. Implement PPU-specific interrupts.
   - VBlank, HBlank, OAM, and LYC/LY=LYC STAT conditions.
   - Correct STAT source and edge behavior.

**Exit gate:** Static screenshots, PPU timing tests, sprites, window, scrolling, and DMG Acid2-style rendering tests pass.

### Phase 7: Implement the Main Loop and SDL2 Frontend

**Goal:** Run the emulator as an interactive application.

1. Implement CLI parsing.
   - Require exactly or optionally accept one ROM path.
   - Print usage and a readable error for invalid arguments.
   - Resolve relative and absolute paths consistently.
2. Implement SDL2 initialization.
   - Window, renderer, texture, event loop, and shutdown.
   - Fixed 160x144 logical texture with a scalable window.
   - Graceful handling of initialization failure.
3. Implement host keyboard mapping.
   - D-pad: arrow keys.
   - A/B: Z/X.
   - Start/Select: Enter/Right Shift or make the mapping configurable.
   - Escape or dedicated action for debugger/menu.
   - Document the final key map in README.
4. Implement frame synchronization.
   - Run until exactly one frame's worth of T-cycles.
   - Render once per frame.
   - Add an adjustable frame limit or VSync policy after correctness is established.
5. Implement clean shutdown.
   - Flush battery RAM and RTC state.
   - Destroy SDL2 resources.
   - Free the emulator state.

**Exit gate:** Tetris and Dr. Mario run with responsive controls and stable frame timing.

### Phase 8: Add External Test ROM Support

**Goal:** Add repeatable automated and visual validation.

1. Add unit tests for MMU regions and bus boundaries.
2. Add CPU tests for every instruction family.
3. Add tests for every conditional branch taken/not-taken cycle count.
4. Add timer tests for DIV, TIMA overflow, TAC, and edge behavior.
5. Add interrupt tests for IME, IF, IE, priority, and pending interrupt behavior.
6. Add PPU tests for scanline, mode, STAT, and LY timing.
7. Add cartridge tests for ROM/RAM banking.
8. Add ROM harness support.
   - Optional external test-ROM path.
   - Serial output capture for Blargg-style test results.
   - Timeout detection for hanging ROMs.
   - Clear pass/fail output.
9. Run CPU instruction, timing, and memory test ROMs in the external test process.

**Exit gate:** Core hardware tests are reproducible from a single test command.

### Phase 9: Implement the Debugger

**Goal:** Make CPU and timing bugs diagnosable.

1. Add a text debugger interface.
   - Pause/resume execution.
   - Step one instruction.
   - Step one T-cycle or frame.
   - Run until PC, read, write, or interrupt.
   - Breakpoints and temporary breakpoints.
2. Display CPU state.
   - PC, SP, AF, BC, DE, HL, IME, halted, stopped, and total cycles.
   - Disassemble the current instruction and nearby bytes.
3. Display memory state.
   - Read and write memory by address.
   - View ROM bank, VRAM, WRAM, OAM, HRAM, IE, and IF.
4. Display cartridge state.
   - Controller, ROM/RAM bank, RAM enable, and RTC state.
5. Display PPU state.
   - LCDC, STAT, LY, LYC, scrolls, window position, and PPU cycle.
6. Add logging controls.
   - Opcode trace.
   - Memory access trace.
   - Interrupt trace.
   - DMA and PPU mode trace.
   - Configurable trace count and output file.

**Exit gate:** A failing instruction or cartridge can be isolated without modifying source.

### Phase 10: Implement Save States

**Goal:** Save and restore the complete deterministic emulator state.

1. Define a versioned binary save-state format.
2. Serialize CPU, interrupt state, timers, PPU, cartridge, MMU, DMA, joypad, serial, and RTC state.
3. Add load validation for version and cartridge identity.
4. Add keyboard shortcuts for save slots.
5. Do not replace battery saves with save states.

**Exit gate:** State saves and loads exactly; one state file can be loaded repeatedly.

### Phase 11: Add GBC Compatibility

**Goal:** Extend, rather than rewrite, the DMG core.

1. Detect CGB cartridges from the cartridge header.
2. Implement CGB boot state and mode selection.
3. Implement CGB VRAM banking.
4. Implement CGB WRAM banking.
5. Implement CGB palette RAM and palettes.
6. Implement CGB sprite and background attributes.
7. Implement CGB DMA and speed switching.
8. Implement CGB timer and interrupt differences.
9. Implement CGB double-speed timing while preserving T-cycle accounting.
10. Preserve DMG compatibility rendering for CGB booted in DMG mode.

**Exit gate:** A CGB cartridge boots and renders correctly without changing DMG behavior.

### Phase 12: Add SGB Compatibility

**Goal:** Support the Super Game Boy peripheral protocol.

1. Detect SGB-compatible cartridge metadata.
2. Implement SGB packet transfer through serial I/O.
3. Implement SGB command handling needed for palettes and borders.
4. Implement SGB border rendering and frame composition.
5. Implement SGB controller/control transfer behavior.
6. Add an initial 256-color host palette representation.

**Exit gate:** SGB-enabled games render and transfer at least the core palette, border, and controller commands.

### Phase 13: Add Audio

**Goal:** Implement the Game Boy APU after the visual core is stable.

1. Implement APU frame sequencer and sample timing.
2. Implement Square Channel 1, including sweep.
3. Implement Square Channel 2.
4. Implement Wave Channel 3.
5. Implement Noise Channel 4.
6. Implement mixer and SDL2 audio callback.
7. Implement APU power, NR registers, DAC, and LENGTH behavior.
8. Implement envelope, frequency, timer, and sample playback timing.
9. Add CGB audio differences only after DMG audio is stable.

**Exit gate:** Test music and effects in Tetris and the Zelda title without underruns or large timing drift.

## 5. Detailed Test Plan

### 5.1 Unit Test Suites

- CPU instruction decode and execution.
- CPU register pair and flag behavior.
- CB-prefixed instructions.
- Memory read/write routing.
- Cartridge MBC behavior.
- Timer edge timing.
- Interrupt timing and priority.
- DMA timing.
- PPU line, mode, LY, STAT, and LY=LYC behavior.
- Joypad transitions.
- Save-state round trips.
- RTC behavior.

### 5.2 ROM Validation

Run the following through a repeatable ROM harness:

- CPU instructions and timing ROM sets.
- Instruction timing and memory access ROM sets.
- PPU timing and LCD behavior ROM sets.
- DMG rendering suite.
- Tetris.
- Dr. Mario.
- The selected Zelda title.

The exact ROM filenames, paths, and pass markers must be documented in a test manifest before final validation.

### 5.3 Manual Playability Checks

- Boot into the title screen.
- Start a new game or load a save.
- Navigate menus.
- Test all D-pad directions.
- Test A, B, Start, and Select.
- Scroll and enter a level.
- Trigger sprites and window objects.
- Let timer-heavy and interrupt-heavy scenes run for at least ten minutes.
- Quit and reload a battery save.
- Test hot reloads or save-state restores if those features are enabled.

## 6. Repository and Quality Requirements

1. Keep public headers small and subsystem-oriented.
2. Keep MMU access in MMU/bus functions instead of direct array access in subsystem code.
3. Use fixed-width integer types for emulated hardware state.
4. Separate host I/O (SDL2, file access) from hardware emulation.
5. Avoid UB for signed arithmetic; explicitly model 8/16-bit wrapping.
6. Document hardware assumptions next to each implementation.
7. No unimplemented opcode may silently appear as NOP.
8. Keep test and release binaries out of source control.
9. Use an existing build system pattern until cross-platform packaging becomes necessary.

## 7. Suggested Deliverables

1. A stable test harness.
2. A DMG core passing CPU and timing tests.
3. A playable SDL2 DMG emulator.
4. Cartridge controller support.
5. Battery saves and save states.
6. A debugger.
7. GBC support.
8. SGB support.
9. Audio.

## 8. Dependencies and Milestones

No game can be considered complete until the preceding subsystem milestone passes. Recommended sequence:

1. Build/test harness.
2. Bus and cartridge loading.
3. CPU.
4. Timers/interrupts/DMA/joypad.
5. PPU.
6. SDL2 loop.
7. Tetris and Dr. Mario.
8. Debugger/save states.
9. GBC.
10. SGB.
11. Audio.