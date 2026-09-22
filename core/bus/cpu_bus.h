#ifndef CPU_BUS_H
#define CPU_BUS_H

#include "cartridge/cartridge.h"
#include "controller/controller.h"
#include "ppu/ppu.h"

#include <array>
#include <cstdint>

// The 6502's 64KB address space. The PPU has a separate one behind $2000-$2007.
class CpuBus {
  std::array<uint8_t, 2048> ram_{};
  Cartridge &cart_;
  PPU &ppu_;
  Controller &pad1_;
  Controller &pad2_;
  int stall_ = 0;

public:
  CpuBus(Cartridge &cart, PPU &ppu, Controller &pad1, Controller &pad2)
      : cart_(cart), ppu_(ppu), pad1_(pad1), pad2_(pad2) {}

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

  // OAM DMA ($4014) freezes the CPU; the caller folds this into cycle counts.
  int drain_stall() {
    const int s = stall_;
    stall_ = 0;
    return s;
  }
};

#endif
