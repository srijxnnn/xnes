#ifndef CPU_BUS_H
#define CPU_BUS_H

#include "controller/controller.h"
#include "mapper/mapper.h"
#include "ppu/ppu.h"

#include <array>
#include <cstdint>

class CpuBus {
public:
  CpuBus(Mapper &mapper, PPU &ppu, Controller &pad1, Controller &pad2)
      : mapper(mapper), ppu(ppu), pad1(pad1), pad2(pad2) {}

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

  int drain_stall() {
    const int drained = stall;
    stall = 0;
    return drained;
  }

private:
  void oam_dma(uint8_t page);

  std::array<uint8_t, 2048> ram{};
  Mapper &mapper;
  PPU &ppu;
  Controller &pad1;
  Controller &pad2;
  int stall = 0;
};

#endif
