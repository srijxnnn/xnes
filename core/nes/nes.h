#ifndef NES_H
#define NES_H

#include "bus/cpu_bus.h"
#include "bus/ppu_bus.h"
#include "cartridge/cartridge.h"
#include "controller/controller.h"
#include "cpu/cpu.h"
#include "mapper/mapper.h"
#include "ppu/ppu.h"

#include <filesystem>
#include <memory>

// The console: owns every component and the wiring between them. Frontends
// talk to this and never assemble the parts themselves.
class NES {
public:
  // Returns nullptr if the file is not a readable iNES image or its board is
  // not implemented. Everything that can fail happens here, so the constructor
  // only ever receives parts that are already valid.
  static std::unique_ptr<NES> load(const std::filesystem::path &rom);

  NES(std::unique_ptr<Cartridge> cart, std::unique_ptr<Mapper> mapper);

  NES(const NES &) = delete;
  NES &operator=(const NES &) = delete;

  void reset();

  // One CPU instruction, then 3 PPU dots per CPU cycle (including DMA stall).
  void step();

  // Run until the PPU finishes the current frame.
  void step_frame();

  // The debug trace steps one instruction at a time and stops on this.
  uint64_t frame() const { return ppu.frame; }

  void set_buttons(uint8_t pad1, uint8_t pad2 = 0);

  bool halted() const { return cpu.halted; }

  const uint32_t *pixels() const { return ppu.pixels.data(); }

private:
  // Declaration order is construction order, and each component is built from
  // references to the ones above it. The cartridge and mapper are held by
  // pointer so that their addresses do not depend on where this object lives.
  std::unique_ptr<Cartridge> cart;
  std::unique_ptr<Mapper> mapper;
  PpuBus ppu_bus;
  PPU ppu;
  Controller pad1;
  Controller pad2;
  CpuBus cpu_bus;

public:
  // The debug trace reads the registers. Stepping stays on NES::step.
  CPU cpu;
};

#endif
