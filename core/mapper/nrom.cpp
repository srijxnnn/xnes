#include "nrom.h"

uint8_t NROM::cpu_read(uint16_t addr) const {
  if (addr >= 0x8000) {
    return cart.prg[(addr - 0x8000) & (cart.prg.size() - 1)];
  }
  if (addr >= 0x6000) {
    return cart.prg_ram[addr - 0x6000];
  }
  return 0;
}

void NROM::cpu_write(uint16_t addr, uint8_t data) {
  if (addr >= 0x6000 && addr < 0x8000) {
    cart.prg_ram[addr - 0x6000] = data;
  }
}

uint8_t NROM::chr_read(uint16_t addr) const {
  return cart.chr[addr & (cart.chr.size() - 1)];
}

void NROM::chr_write(uint16_t addr, uint8_t data) {
  if (cart.chr_ram) {
    cart.chr[addr & (cart.chr.size() - 1)] = data;
  }
}
