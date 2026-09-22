# Architecture

XNES is two halves that meet at a single class, `NES`.

- `core/` is the emulator. There is no Qt include anywhere in it.
- `ui/` is the frontend. It only ever touches `NES`, never a component.

```
core/
  nes/         the console: owns every part, wires them, steps them
  cpu/         6502 (cpu.cpp is behaviour, cpu_table.cpp is the decode table)
  ppu/         registers, sprites, scroll, scanline renderer
  cartridge/   the parsed iNES image: ROM, work RAM, what the header claims
  mapper/      how a board maps that image; one subclass per iNES mapper
  bus/         the two address spaces: cpu_bus and ppu_bus
  controller/  the pad behind $4016/$4017
ui/
  game_view/   the picture and the keyboard
  debug_log/   the instruction window
  trace/       the latest instructions, formatted in one place
  window/      toolbar: owns the console and steps it
  main/        CLI: play a ROM
```

A directory is one concept, declared in a header and implemented in the `.cpp`.
Include paths are rooted at `core/`, which is why they read `cpu/cpu.h`.

The two buses are the seams. Nothing reaches past them: the CPU only knows
`CpuBus`, the renderer only knows `PpuBus`, and both buses are the only things
that know a cartridge exists.

## The clock

There is no scheduler and no central cycle counter. The CPU is the clock source
and the PPU follows it.

`NES::step()` runs one instruction, asks the CPU how many cycles it took, and
ticks the PPU three dots per cycle. OAM DMA is part of that count: a `$4014`
write reports its 513 stall cycles back through the bus, and `CPU::step` folds
them into its return value, so the PPU sees the stall without knowing about it.
A dot that raises vblank NMI latches it on the CPU, which takes it before the
next instruction. `NES::step_frame()` repeats until the PPU's frame counter
moves.

The consequence is that the CPU may run up to one instruction ahead of the PPU.
That is fine for NROM games and is the first thing to revisit if a game needs
tighter timing.

## Ownership

`NES` owns every component and nothing owns anything else; the buses and the
PPU hold references because `NES` is the owner. Member order in `nes.h` is
load-bearing, because each component is constructed from references to the ones
declared above it: mapper, then PPU bus, then PPU, then CPU bus, then CPU.

The cartridge and mapper are the two exceptions, held by `unique_ptr`. The
mapper stores a `Cartridge &`, so the cartridge needs an address that does not
move when the `NES` itself is moved or returned.

Everything that can fail happens in `NES::load`: it parses the image, asks for
a mapper, and only then constructs an `NES`. The constructor therefore takes
parts that are already known to be valid and cannot fail, which is why there is
no half-built console to check for.

## Adding a mapper

This is the one axis the emulator is expected to grow along, so it is the one
place with a virtual interface.

`Mapper` is the board: it answers CPU reads and writes above `$4020`, CHR reads
and writes below `$2000`, and reports nametable mirroring. `Cartridge` holds the
bytes and knows nothing about addresses. To add a board, subclass `Mapper` and
add one case to `create_mapper`, which is the only function aware of which
mappers exist. No bus, PPU or CPU code changes.

That interface costs about 15% of throughput, because a CHR read is a virtual
call and rendering does two per pixel. It is worth measuring before caring:
Donkey Kong runs at roughly 700 frames a second, twelve times faster than it
needs to. If it ever does matter, the usual fix is for the mapper to hand the
renderer a pointer to the current CHR bank instead of answering byte by byte.

## Where the PPU approximates

Dots are counted exactly, so vblank, NMI and sprite-0 are raised on the right
cycle, and the scroll reloads at dots 257 and 280 make a mid-frame `$2005`
write take effect on the following scanline.

Pixels, however, are produced a whole scanline at a time, from the registers as
they stand at dot 1 of that line, rather than one pixel per dot. Register
changes *within* a scanline are therefore missed. Donkey Kong's status bar
works because it splits between lines, not inside one. Anything needing a
mid-line split needs this loop moved into `tick`.

## Deliberate gaps

NROM is the only board implemented, so `create_mapper` returns nullptr for
anything else and `NES::load` reports the ROM as unloadable rather than
mis-emulating it. No APU, so `CpuBus` drops the writes it cannot decode and the
machine stays silent. No save states.

## The debug trace

The toolbar's Debug action opens a second window. While it is open, each
instruction is stored before `NES::step` and the window shows the latest
lines (`PC A X Y P SP CYC`) once per frame. Inserting every instruction
into the widget would miss the frame time. With the window closed the game
stays on `NES::step_frame`.
