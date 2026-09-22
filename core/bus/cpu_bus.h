#ifndef CPU_BUS_H
#define CPU_BUS_H

#include "controller/controller.h"
#include "mapper/mapper.h"
#include "ppu/ppu.h"

#include <array>
#include <cstdint>

// The 6502's 64KB address space. Everything it can reach is either the 2KB of
// internal RAM held here or a device referenced here; the devices themselves are
// owned by NES.
class CpuBus {
public:
  CpuBus(Mapper &mapper, PPU &ppu, Controller &pad1, Controller &pad2)
      : mapper_(mapper), ppu_(ppu), pad1_(pad1), pad2_(pad2) {}

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

  // OAM DMA freezes the CPU; the caller folds this into its cycle counts.
  int drain_stall() {
    const int stall = stall_;
    stall_ = 0;
    return stall;
  }

private:
  // $4014: copies a 256-byte page of CPU memory into OAM and stalls the CPU.
  void oam_dma_(uint8_t page);

  std::array<uint8_t, 2048> ram_{};
  Mapper &mapper_;
  PPU &ppu_;
  Controller &pad1_;
  Controller &pad2_;
  int stall_ = 0;
};

#endif
