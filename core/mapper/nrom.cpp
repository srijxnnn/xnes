#include "nrom.h"

uint8_t NROM::cpu_read(uint16_t addr) const {
  // PRG is a power of two, so masking is the mirroring.
  if (addr >= 0x8000) {
    return cart_.prg((addr - 0x8000) & (cart_.prg_size() - 1));
  }
  if (addr >= 0x6000) {
    return cart_.prg_ram(addr - 0x6000);
  }
  return 0;
}

void NROM::cpu_write(uint16_t addr, uint8_t data) {
  // There are no registers to write: only the work RAM accepts anything.
  if (addr >= 0x6000 && addr < 0x8000) {
    cart_.set_prg_ram(addr - 0x6000, data);
  }
}

uint8_t NROM::chr_read(uint16_t addr) const {
  return cart_.chr(addr & (cart_.chr_size() - 1));
}

void NROM::chr_write(uint16_t addr, uint8_t data) {
  if (cart_.chr_is_ram()) {
    cart_.set_chr(addr & (cart_.chr_size() - 1), data);
  }
}
