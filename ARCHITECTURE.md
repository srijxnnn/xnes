# Architecture

XNES is two halves that meet at a single class, `NES`.

- `core/` is the emulator. There is no Qt include anywhere in it, so it builds
  and runs headless. That is what makes the nestest trace and `--dump-ppm`
  possible without a display.
- `ui/` is the frontend. It only ever touches `NES`, never a component.

```
core/
  nes/         the console: owns every part, wires them, steps them
  cpu/         6502, including the unofficial opcodes nestest runs
  ppu/         scanline renderer, vblank/NMI, sprite-0 hit
  cartridge/   iNES parsing, NROM
  bus/         the CPU's $0000-$FFFF
  controller/  the pad behind $4016/$4017
ui/
  window       Qt widget: blit the frame, read the keyboard
  main         CLI: play a ROM, trace nestest, or dump a PPM
```

Each directory is one class, declared in the header and implemented in the
`.cpp`. Include paths are rooted at `core/`, which is why they read `cpu/cpu.h`.

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

`NES` holds every component as a direct member, so the machine is one
allocation and there is no dynamic wiring. Member order in `nes.h` is
load-bearing: each component is constructed with references to the ones
declared before it, hence cartridge before PPU, and bus before CPU. Components
never own each other; `CpuBus` holds references because `NES` is the owner.

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

Mapper 0 only; `Cartridge::load` rejects anything else rather than
mis-emulating it. No APU, so `CpuBus` drops the writes it cannot decode and the
machine stays silent. No save states.

## The correctness contract

`test/run_nestest.sh` runs the CPU over `nestest.nes` and diffs its trace
against the golden log, so any change to the CPU has to leave thousands of
lines of register and cycle state byte-identical. Run it with `ctest
--test-dir build`. The script trims the golden log to the fields the trace
prints, since disassembly and PPU columns are not emitted yet.

For the picture, `xnes --dump-ppm <frames> <out.ppm> <rom>` renders without a
window, which is the cheapest way to check a PPU change against a known frame.
