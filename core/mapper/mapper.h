#ifndef MAPPER_H
#define MAPPER_H

#include "cartridge/cartridge.h"

#include <cstdint>
#include <memory>

class Mapper {
public:
  explicit Mapper(Cartridge &cart) : cart(cart) {}
  virtual ~Mapper() = default;

  Mapper(const Mapper &) = delete;
  Mapper &operator=(const Mapper &) = delete;

  virtual uint8_t cpu_read(uint16_t addr) const = 0;
  virtual void cpu_write(uint16_t addr, uint8_t data) = 0;

  virtual uint8_t chr_read(uint16_t addr) const = 0;
  virtual void chr_write(uint16_t addr, uint8_t data) = 0;

  virtual Mirror mirror() const { return cart.mirror; }

protected:
  Cartridge &cart;
};

std::unique_ptr<Mapper> create_mapper(Cartridge &cart);

#endif
