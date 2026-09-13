#ifndef BUS_H
#define BUS_H

#include "cartridge.h"
#include <array>

class Bus {
  std::array<uint8_t, 2048> ram_{}; // 2KB RAM
  Cartridge *cart_ = nullptr;

public:
  void insert(Cartridge &c) { cart_ = &c; }
  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);
};

#endif