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
    : cart_(std::move(cart)), mapper_(std::move(mapper)), ppu_bus_(*mapper_),
      ppu_(ppu_bus_), cpu_bus_(*mapper_, ppu_, pad1_, pad2_), cpu_(cpu_bus_) {
  reset();
}

void NES::reset() {
  ppu_bus_.reset();
  ppu_.reset();
  cpu_.reset();
}

void NES::step() {
  const int cycles = cpu_.step();
  for (int i = 0; i < cycles * 3; i++) {
    ppu_.tick();
    if (ppu_.take_nmi()) {
      cpu_.nmi();
    }
  }
}

void NES::step_frame() {
  const uint64_t frame = ppu_.frame();
  while (ppu_.frame() == frame && !cpu_.halted()) {
    step();
  }
}

void NES::set_buttons(uint8_t pad1, uint8_t pad2) {
  pad1_.set(pad1);
  pad2_.set(pad2);
}
