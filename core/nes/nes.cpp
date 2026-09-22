#include "nes.h"

#include <utility>

std::unique_ptr<NES> NES::load(const std::filesystem::path &rom) {
  auto cart = Cartridge::load(rom);
  if (!cart) {
    return nullptr;
  }

  return std::make_unique<NES>(std::move(cart.value()));
}

NES::NES(Cartridge cart)
    : cart_(std::move(cart)), ppu_(cart_), bus_(cart_, ppu_, pad1_, pad2_),
      cpu_(bus_) {
  reset();
}

void NES::reset() {
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
