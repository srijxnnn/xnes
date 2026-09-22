# Working on XNES

An NES emulator in C++17. `core/` is the machine, `ui/` is a Qt frontend.

Read [ARCHITECTURE.md](ARCHITECTURE.md) before changing anything structural; it
covers the clock, ownership, and the two buses. This file is only about how to
work in the repo without breaking it.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Verify before you claim anything works

This is an emulator, so "it compiles" and "it looks right" mean very little. A
subtly wrong cycle count or palette index still renders a plausible picture.
There is no automated CPU or picture test.

The toolbar's Debug action opens a second window with the latest
instructions (`PC A X Y P SP CYC`). A full log is thousands of lines a
frame, so that window keeps a short tail and is refreshed once per frame.
With the window closed the game stays on `NES::step_frame`.

## Hard rules

- **No Qt in `core/`.** Not even a transitive include. The window and the
  debug trace both live in `ui/`.
- **`ui/` talks to `NES` and nothing else.** It never constructs a component or
  reaches through to one.
- **Never commit ROMs.** `test/roms/*` is gitignored. `nestest.nes` and
  `nestest.log` are the only test data in the repo; supply anything else
  locally.
- **Don't add a mapper by editing `Cartridge`.** Subclass `Mapper` and add one
  case to `create_mapper`. `Cartridge` holds bytes and header fields; it does
  not know what `$8000` means.

## Style

No `.clang-format` is committed, so match the surrounding code: LLVM defaults,
two-space indent, 80 columns, braces on every `if` body.

- Constants are `kCamelCase`.
- Headers use `#ifndef` guards, not `#pragma once`.
- Failure is `std::optional` or `nullptr`. The codebase does not throw.
- Hardware register bits get named enum values, never bare masks. `mask &
  ShowBg`, not `mask & 0x08`.
- Comment constraints the code cannot show — a hardware quirk, a reason an
  ordering matters. Do not narrate what the next line does, and do not leave
  notes about the change itself; those read as noise once merged.

## Performance

`CPU::step`, `CpuBus::read`/`write`, and everything reachable from
`PPU::render_scanline` are hot. `background_dot` alone runs 61,440 times per
frame. Do not add allocation, `std::function`, or virtual dispatch in there.

`Mapper` is the deliberate exception: its virtual CHR read costs roughly 15% of
throughput and is worth it, because mappers are the one axis this project has to
grow along. Measure before adding a second exception.

## Known inaccuracies — do not silently "fix" these

They are deliberate or tracked, and touching them changes rendered output:

- Pixels are produced a whole scanline at a time from the registers at dot 1,
  not per dot. Mid-scanline register writes are missed.
- `sprite_dot` reports a sprite-0 hit only when sprite 0 is the frontmost
  opaque sprite. Hardware flags the overlap even when another sprite covers it.
  This is a real bug, still open.
- APU writes are dropped, so the machine is silent.
- `Instruction::name` in the decode table is currently unread. It keeps the
  mnemonic, including the `*` on unofficial opcodes. Leave it.

## Where things go

- Opcode behaviour lives in `core/cpu/cpu.cpp`; an opcode's addressing mode,
  cycle count, or mnemonic lives in `core/cpu/cpu_table.cpp`. The table is data
  and is kept apart on purpose.
- Unofficial opcodes are required, not optional: the decode table implements
  them.
- PPU memory, mirroring, and palette folding belong in `PpuBus`, not `PPU`.
