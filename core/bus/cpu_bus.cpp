#include "cpu_bus.h"

uint8_t CpuBus::read(uint16_t addr) {
  if (addr < 0x2000) {
    return ram_[addr & 0x07FF];
  }
  if (addr < 0x4000) {
    return ppu_.cpu_read(addr);
  }
  if (addr == 0x4016) {
    return pad1_.read();
  }
  if (addr == 0x4017) {
    return pad2_.read();
  }
  if (addr >= 0x4020) {
    return cart_.cpu_read(addr);
  }
  return 0;
}

void CpuBus::write(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_[addr & 0x07FF] = data;
    return;
  }
  if (addr < 0x4000) {
    ppu_.cpu_write(addr, data);
    return;
  }
  if (addr == 0x4014) {
    const uint16_t page = static_cast<uint16_t>(data) << 8;
    for (int i = 0; i < 256; i++) {
      ppu_.oam_write(read(page + static_cast<uint16_t>(i)));
    }
    stall_ += 513;
    return;
  }
  if (addr == 0x4016) {
    pad1_.write(data);
    pad2_.write(data);
    return;
  }
  if (addr >= 0x4020) {
    cart_.cpu_write(addr, data);
  }
}
