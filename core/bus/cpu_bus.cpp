#include "cpu_bus.h"

uint8_t CpuBus::read(uint16_t addr) {
  if (addr < 0x2000) {
    return ram[addr & 0x07FF];
  }
  if (addr < 0x4000) {
    return ppu.cpu_read(addr);
  }
  if (addr == 0x4016) {
    return pad1.read();
  }
  if (addr == 0x4017) {
    return pad2.read();
  }
  if (addr >= 0x4020) {
    return mapper.cpu_read(addr);
  }
  return 0;
}

void CpuBus::write(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram[addr & 0x07FF] = data;
    return;
  }
  if (addr < 0x4000) {
    ppu.cpu_write(addr, data);
    return;
  }
  if (addr == 0x4014) {
    oam_dma(data);
    return;
  }
  if (addr == 0x4016) {
    pad1.write(data);
    pad2.write(data);
    return;
  }
  if (addr >= 0x4020) {
    mapper.cpu_write(addr, data);
  }
}

void CpuBus::oam_dma(uint8_t page) {
  const uint16_t base = static_cast<uint16_t>(page) << 8;
  for (int i = 0; i < 256; i++) {
    ppu.oam_write(read(base + static_cast<uint16_t>(i)));
  }
  stall += 513;
}
