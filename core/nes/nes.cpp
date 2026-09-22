#include "nes.h"

#include <utility>

std::unique_ptr<NES> NES::load(const std::filesystem::path &rom) {
  auto image = Cartridge::load(rom);
  if (!image) {
    return nullptr;
  }

  auto cart = std::make_unique<Cartridge>(std::move(*image));
  auto mapper = create_mapper(*cart);
  if (!mapper) {
    return nullptr;
  }

  return std::make_unique<NES>(std::move(cart), std::move(mapper));
}

NES::NES(std::unique_ptr<Cartridge> cart, std::unique_ptr<Mapper> mapper)
    : cart(std::move(cart)), mapper(std::move(mapper)),
      ppu_bus(*this->mapper), ppu(ppu_bus),
      cpu_bus(*this->mapper, ppu, pad1, pad2), cpu(cpu_bus) {
  reset();
}

void NES::reset() {
  ppu_bus.reset();
  ppu.reset();
  cpu.reset();
}

void NES::step() {
  const int cycles = cpu.step();
  for (int i = 0; i < cycles * 3; i++) {
    ppu.tick();
    if (ppu.take_nmi()) {
      cpu.nmi();
    }
  }
}

void NES::step_frame() {
  const uint64_t frame = ppu.frame;
  while (ppu.frame == frame && !cpu.halted) {
    step();
  }
}

void NES::set_buttons(uint8_t pad1, uint8_t pad2) {
  this->pad1.set(pad1);
  this->pad2.set(pad2);
}
