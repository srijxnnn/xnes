#ifndef PPU_BUS_H
#define PPU_BUS_H

#include "mapper/mapper.h"

#include <array>
#include <cstdint>

// The PPU's own address space, the counterpart to CpuBus: $0000-$1FFF is the
// cartridge's pattern tables, $2000-$3EFF the nametables folded according to
// the board's mirroring, and $3F00-$3FFF palette RAM with its own folding.
class PpuBus {
public:
  explicit PpuBus(Mapper &mapper) : mapper_(mapper) {}

  void reset();

  uint8_t read(uint16_t addr) const;
  void write(uint16_t addr, uint8_t data);

  // Rendering goes straight at the two things it needs per pixel, because
  // routing them through read() would redo the address decode every time.
  uint8_t pattern(uint16_t addr) const { return mapper_.chr_read(addr); }
  uint8_t colour(uint8_t index) const { return palette_[index] & 0x3F; }

private:
  // Two physical nametables are folded into four slots; four-screen boards
  // carry their own RAM and use all of it, hence 4KB rather than 2KB.
  uint16_t nametable_index_(uint16_t addr) const;

  // $3F10/$3F14/$3F18/$3F1C are aliases of the backdrop at $3F00.
  static uint8_t palette_index_(uint16_t addr);

  Mapper &mapper_;
  std::array<uint8_t, 4096> nametable_{};
  std::array<uint8_t, 32> palette_{};
};

#endif
