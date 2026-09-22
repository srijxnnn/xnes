#ifndef NES_H
#define NES_H

#include "bus/cpu_bus.h"
#include "cartridge/cartridge.h"
#include "controller/controller.h"
#include "cpu/cpu.h"
#include "ppu/ppu.h"

#include <filesystem>
#include <memory>

// The console: owns every component and the wiring between them. Frontends
// talk to this and never assemble the parts themselves.
class NES {
public:
  static std::unique_ptr<NES> load(const std::filesystem::path &rom);

  explicit NES(Cartridge cart);

  NES(const NES &) = delete;
  NES &operator=(const NES &) = delete;

  void reset();

  // One CPU instruction, then 3 PPU dots per CPU cycle (including DMA stall).
  void step();

  // Run until the PPU finishes the current frame.
  void step_frame();

  void set_buttons(uint8_t pad1, uint8_t pad2 = 0);

  bool halted() const { return cpu_.halted(); }

  const uint32_t *pixels() const { return ppu_.pixels(); }

  // The nestest trace needs the registers, and it drives the CPU directly.
  CPU &cpu() { return cpu_; }

private:
  // Declaration order is construction order: PPU and pads before the bus,
  // bus before the CPU.
  Cartridge cart_;
  PPU ppu_;
  Controller pad1_;
  Controller pad2_;
  CpuBus bus_;
  CPU cpu_;
};

#endif
