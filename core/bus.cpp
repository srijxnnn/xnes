#include "bus.h"

uint8_t Bus::read(uint16_t addr) {
  if (addr < 0x2000) {
    return ram_[addr & 0x07FF];
  }

  if (cart_ && addr >= 0x8000) {
    const auto &prg = cart_->prg();
    return prg[addr & 0x3FFF];
  }

  return 0;
}

void Bus::write(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_[addr & 0x07FF] = data;
  }

  return;
}