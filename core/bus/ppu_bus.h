#ifndef PPU_BUS_H
#define PPU_BUS_H

#include "mapper/mapper.h"

#include <array>
#include <cstdint>

class PpuBus {
public:
  explicit PpuBus(Mapper &mapper) : mapper(mapper) {}

  void reset();

  uint8_t read(uint16_t addr) const;
  void write(uint16_t addr, uint8_t data);

  uint8_t pattern(uint16_t addr) const { return mapper.chr_read(addr); }
  uint8_t colour(uint8_t index) const { return palette[index] & 0x3F; }

private:
  uint16_t nametable_index(uint16_t addr) const;
  static uint8_t palette_index(uint16_t addr);

  Mapper &mapper;
  std::array<uint8_t, 4096> nametable{};
  std::array<uint8_t, 32> palette{};
};

#endif
