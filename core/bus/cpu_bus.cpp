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
    return mapper_.cpu_read(addr);
  }
  // The APU lives in the gap. Reads there come back as 0.
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
    oam_dma_(data);
    return;
  }
  if (addr == 0x4016) {
    pad1_.write(data);
    pad2_.write(data);
    return;
  }
  if (addr >= 0x4020) {
    mapper_.cpu_write(addr, data);
  }
  // Anything left is the APU, which is dropped, so the machine stays silent.
}

void CpuBus::oam_dma_(uint8_t page) {
  const uint16_t base = static_cast<uint16_t>(page) << 8;
  for (int i = 0; i < 256; i++) {
    ppu_.oam_write(read(base + static_cast<uint16_t>(i)));
  }
  stall_ += 513;
}
