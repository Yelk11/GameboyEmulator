
# Resources


[Pan Docs](https://gbdev.io/pandocs/?utm_source=chatgpt.com) - This is the best documentation on the gameboy




# Plan
I need to figure out the general archecture of the GBA
I need to create a build system
I need to develop a test plan/suite





## Archetecture

MMU - Routes reads and writes acrss the 16 bit address space, enforces memory aces and runs OAM DMA transfers
CPU - reads and executes instrcutions from MMU
PPU - renders scanlines in 160x144 framebuffer (reads vram and OAM)
APU - synthesisze audio across four channels (2 square waves, 1 wave tabe, 1 noise)
MBC - laods ROM data, manages external save RAM and handles memory bank controllers
Timers - Increments divider and programmable timers (DIV, TIMA) to fire timer interrupts, and maps button states into the P1/JOYP register.

## interactiosn
1. The Main Loop tells the CPU to execute one instruction.
2. The CPU reads/writes bytes via the Memory Bus and returns how many clock cycles (T-cycles) the instruction took.
3. The PPU, APU, and Timers advance by that exact number of cycles.
4. When the PPU finishes drawing line 144, it triggers a VBlank interrupt and signals the Frontend to render the completed frame to the screen.




## CPU
read
execute
write



## Memory