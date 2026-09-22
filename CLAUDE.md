# Working on XNES

An NES emulator in C++17. `core/` is the machine, `ui/` is a Qt frontend.

Read [ARCHITECTURE.md](ARCHITECTURE.md) before changing anything structural; it
covers the clock, ownership, and the two buses. This file is only about how to
work in the repo without breaking it.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build
```

## Verify before you claim anything works

This is an emulator, so "it compiles" and "it looks right" mean very little. A
subtly wrong cycle count or palette index still renders a plausible picture.
There are two checks, and they cover different halves.

**The CPU is guarded.** `ctest` runs the trace over `nestest.nes` and diffs
thousands of lines of register and cycle state against a golden log. Any CPU
change has to keep it passing. If it fails, read the first differing line: the
fields are `PC A X Y P SP CYC`, so a `CYC` drift means timing and a register
drift means behaviour.

**The PPU is not guarded.** There is no automated rendering test, so you have to
make one for the change you are about to make:

```bash
# before touching anything
cmake --build build -j && ./build/xnes --dump-ppm 120 /tmp/before.ppm "<rom>"
# after
cmake --build build -j && ./build/xnes --dump-ppm 120 /tmp/after.ppm "<rom>"
cmp /tmp/before.ppm /tmp/after.ppm && echo "identical"
```

Capture the baseline from unmodified code rather than trusting a hash written
down somewhere, since the bytes depend on which ROM dump you used. For
reference, 120 frames of *Donkey Kong (World) (Rev A)* currently gives
`c1976c01f09b45137e0f6c1130009c78`.

A refactor must produce identical bytes. If a change is *meant* to alter the
picture, re-base the baseline deliberately and say in the commit message what
changed and why — do not let an unexplained diff through.

## Hard rules

- **No Qt in `core/`.** Not even a transitive include. The headless build is
  what makes `--nestest` and `--dump-ppm` work, and those are the only ways to
  check the emulator without a display.
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

- Private members and private methods end in `_`. Constants are `kCamelCase`.
- Headers use `#ifndef` guards, not `#pragma once`.
- Failure is `std::optional` or `nullptr`. The codebase does not throw.
- Hardware register bits get named enum values, never bare masks. `mask_ &
  ShowBg`, not `mask_ & 0x08`.
- Comment constraints the code cannot show — a hardware quirk, a reason an
  ordering matters. Do not narrate what the next line does, and do not leave
  notes about the change itself; those read as noise once merged.

## Performance

`CPU::step`, `CpuBus::read`/`write`, and everything reachable from
`PPU::render_scanline_` are hot. `background_dot_` alone runs 61,440 times per
frame. Do not add allocation, `std::function`, or virtual dispatch in there.

`Mapper` is the deliberate exception: its virtual CHR read costs roughly 15% of
throughput and is worth it, because mappers are the one axis this project has to
grow along. Measure before adding a second exception.

## Known inaccuracies — do not silently "fix" these

They are deliberate or tracked, and touching them changes rendered output:

- Pixels are produced a whole scanline at a time from the registers at dot 1,
  not per dot. Mid-scanline register writes are missed.
- `sprite_dot_` reports a sprite-0 hit only when sprite 0 is the frontmost
  opaque sprite. Hardware flags the overlap even when another sprite covers it.
  This is a real bug, still open.
- APU writes are dropped, so the machine is silent.
- `Instruction::name` in the decode table is currently unread. It is the
  mnemonic column the nestest trace still needs, including the `*` on unofficial
  opcodes. Leave it.

## Where things go

- Opcode behaviour lives in `core/cpu/cpu.cpp`; an opcode's addressing mode,
  cycle count, or mnemonic lives in `core/cpu/cpu_table.cpp`. The table is data
  and is kept apart on purpose.
- Unofficial opcodes are required, not optional: nestest executes them.
- PPU memory, mirroring, and palette folding belong in `PpuBus`, not `PPU`.
