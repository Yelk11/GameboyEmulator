Building a Game Boy (DMG) emulator in C is a classic systems project because the hardware is clean, well-documented, and small enough for one person to implement completely.
At its simplest, a working Game Boy emulator is just a clock-synchronized loop passing data between four core subsystems. To get your very first ROM (like Tetris) booting and playable, you only need to build the absolute minimum version of each block—and you can completely skip audio at first.

1. The Memory Map (MMU)

The Game Boy has a 16-bit address bus, meaning it sees 64 KB (0x0000 to 0xFFFF) of unified memory. Everything—ROM, RAM, video memory, and hardware registers—is accessed by reading or writing to specific addresses in this space.
For your simplest version, create two functions: uint8_t read_byte(uint16_t addr) and void write_byte(uint16_t addr, uint8_t val).

Address Range	Region	Simplest Implementation
0x0000 - 0x7FFF	Cartridge ROM	Load a 32 KB ROM file directly into a uint8_t rom[0x8000] array. Ignore bank switching (MBCs) at first—games like Tetris fit entirely in 32 KB.
0x8000 - 0x9FFF	Video RAM (VRAM)	An 8 KB array where tiles and background maps live.
0xC000 - 0xDFFF	Work RAM (WRAM)	An 8 KB general-purpose RAM array for the game's variables.
0xFE00 - 0xFE9F	OAM (Sprite Table)	160 bytes holding position and tile data for up to 40 sprites.
0xFF00 - 0xFF7F	I/O Registers	Joypad state, timer counters, and PPU status/scroll registers.
0xFF80 - 0xFFFE	High RAM (HRAM)	127 bytes of fast internal RAM (often used for the stack).
0xFFFF	Interrupt Enable	A single byte controlling which hardware interrupts are active.


2. The CPU (Sharp LR35902)

The CPU is a hybrid between an Intel 8080 and a Zilog Z80. It has 8-bit registers (A, F, B, C, D, E, H, L) that can also be paired into 16-bit registers (AF, BC, DE, HL), plus a 16-bit Stack Pointer (SP) and Program Counter (PC).
• The F (Flags) Register: Only uses the top 4 bits: Z (Zero), N (Subtract), H (Half-Carry), and C (Carry). Getting Half-Carry right is critical for games to run properly.
• The Instruction Loop: Fetch the byte at PC, increment PC, decode the opcode (usually via a massive switch statement or a 256-entry function pointer table), execute it, and return how many clock cycles it took.
• Start Small: There are 256 standard opcodes and 256 0xCB-prefixed bit-manipulation opcodes. You don't need to write all 512 before running code—implement basic loads (LD), jumps (JP, JR), ALU math (ADD, SUB, INC, DEC, CP), and stack ops (PUSH, POP), then test against Blargg’s cpu_instrs test ROMs.

3. The PPU (Picture Processing Unit)

The PPU renders a 160×144 pixel screen at ~60 Hz using 8×8 pixel tiles (2 bits per pixel = 4 shades of gray). Instead of drawing the whole screen at once, it renders one scanline (LY from 0 to 153) at a time.
For the simplest working graphics, build a scanline renderer:
1. Timing: Each scanline takes 456 clock cycles. Lines 0–143 are visible; lines 144–153 are VBlank (when the game safely updates graphics).
2. Background Layer: Look up tile IDs in the 32×32 background map, fetch the 16 bytes of pixel data for that tile from VRAM, offset by the scroll registers (SCX, SCY), and write 160 pixels into your framebuffer.
3. Sprites (Objects): Loop through the 40 entries in OAM (0xFE00), find up to 10 sprites that intersect the current scanline (LY), and draw their non-transparent pixels on top of the background.
4. Skip at first: You can temporarily ignore the Window layer and cycle-accurate PPU mode transitions (OAM Search / Pixel Transfer) until you see a static title screen.

4. Timers, Joypad & Interrupts

Hardware events pull the CPU out of its normal flow using 5 interrupts (VBlank, LCD STAT, Timer, Serial, Joypad).
• Interrupts: When an event happens (e.g., LY hits 144), set the corresponding bit in the Interrupt Flag (IF at 0xFF0F). Between CPU instructions, if the Master Interrupt Enable (IME) is on and (IE & IF) != 0, push PC onto the stack and jump to the interrupt's fixed handler address (e.g., 0x0040 for VBlank).
• Joypad (0xFF00): Games write a bit to select either D-Pad or Action buttons, and your read_byte(0xFF00) returns which keys are currently held down (active-low: 0 = pressed, 1 = released).
• Timers: Increment the DIV (0xFF04) and TIMA (0xFF05) counters based on elapsed CPU cycles. Many games (like Tetris) read DIV as a source of randomness for piece generation.

The Master Loop (Putting It Together)

Once you hook up a lightweight C library like SDL2 or raylib to open a window and draw a 160x144 pixel array, your main loop looks like this:
c
const int CYCLES_PER_FRAME = 70224; // 456 cycles * 154 scanlines

while (running) {
    int cycles_this_frame = 0;

    while (cycles_this_frame < CYCLES_PER_FRAME) {
        int cycles = cpu_step();          // Execute 1 opcode
        timers_update(cycles);            // Advance DIV and TIMA
        ppu_update(cycles);               // Advance scanlines & set VBlank
        cycles += cpu_handle_interrupts();// Jump to ISR if triggered
        cycles_this_frame += cycles;
    }

    render_framebuffer_to_screen();       // Blit 160x144 texture via SDL/raylib
    poll_keyboard_input();                // Update joypad state & cap to ~60 FPS
}
Use code with caution.

Recommended Build Order

1. Memory + CPU skeleton (skip the boot ROM by initializing registers to their post-boot values, starting PC at 0x0100).
2. Pass Blargg’s CPU test ROMs (print serial port output at 0xFF01/0xFF02 to your terminal so you can see test results before building graphics!).
3. Background-only scanline PPU (gets Tetris or Dr. Mario showing a title screen).
4. Joypad + VBlank Interrupt (lets you press Start and play).
5. Sprites + Timer Interrupts (makes games fully playable).
Would you like to dive deeper into CPU instruction decoding and flag math, how 2bpp tile data maps to pixels in the PPU, or setting up the post-boot initial state so you can jump straight to 0x0100?
