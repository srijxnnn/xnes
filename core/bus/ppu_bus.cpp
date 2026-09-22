#include "ppu_bus.h"

void PpuBus::reset() {
  nametable.fill(0);
  palette.fill(0);
}

uint8_t PpuBus::read(uint16_t addr) const {
  addr &= 0x3FFF;
  if (addr < 0x2000) {
    return mapper.chr_read(addr);
  }
  if (addr < 0x3F00) {
    return nametable[nametable_index(addr)];
  }
  return palette[palette_index(addr)];
}

void PpuBus::write(uint16_t addr, uint8_t data) {
  addr &= 0x3FFF;
  if (addr < 0x2000) {
    mapper.chr_write(addr, data);
    return;
  }
  if (addr < 0x3F00) {
    nametable[nametable_index(addr)] = data;
    return;
  }
  const uint8_t index = palette_index(addr);
  palette[index] = data;
  if ((index & 0x03) == 0) {
    palette[index ^ 0x10] = data;
  }
}

uint16_t PpuBus::nametable_index(uint16_t addr) const {
  addr &= 0x0FFF;
  switch (mapper.mirror()) {
  case Mirror::Vertical:
    return addr & 0x07FF;
  case Mirror::Horizontal:
    return static_cast<uint16_t>(((addr >> 1) & 0x0400) | (addr & 0x03FF));
  case Mirror::Four:
    return addr;
  }
  return addr & 0x07FF;
}

uint8_t PpuBus::palette_index(uint16_t addr) {
  addr &= 0x1F;
  if ((addr & 0x13) == 0x10) {
    addr &= ~0x10;
  }
  return static_cast<uint8_t>(addr);
}
