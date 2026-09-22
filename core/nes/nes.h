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
  static std::unique_ptr<NES> load(const std::filesystem::path &rom);

  NES(std::unique_ptr<Cartridge> cart, std::unique_ptr<Mapper> mapper);

  NES(const NES &) = delete;
  NES &operator=(const NES &) = delete;

  void reset();
  void step();
  void step_frame();
  uint64_t frame() const { return ppu.frame; }

  void set_buttons(uint8_t pad1, uint8_t pad2 = 0);
  bool halted() const { return cpu.halted; }

  const uint32_t *pixels() const { return ppu.pixels.data(); }

private:
  std::unique_ptr<Cartridge> cart;
  std::unique_ptr<Mapper> mapper;
  PpuBus ppu_bus;
  PPU ppu;
  Controller pad1;
  Controller pad2;
  CpuBus cpu_bus;

public:
  CPU cpu;
};

#endif
