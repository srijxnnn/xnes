#ifndef MAPPER_H
#define MAPPER_H

#include "cartridge/cartridge.h"

#include <cstdint>
#include <memory>

// How a board maps the CPU's $4020-$FFFF and the PPU's $0000-$1FFF onto the
// cartridge's ROM and RAM. One subclass per iNES mapper number. The rest of
// core/ talks to this interface and never to a specific board, so supporting a
// new game is a new subclass rather than an edit to the bus or the PPU.
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

  // Boards that switch nametable mirroring at runtime override this. Most just
  // report what the header wired, which is why this is not pure virtual.
  virtual Mirror mirror() const { return cart.mirror; }

protected:
  Cartridge &cart;
};

// Builds the board the cartridge asks for, or nullptr if it is not implemented.
// This is the only place that knows which mappers exist, so adding one means
// one new case here and no change anywhere else.
std::unique_ptr<Mapper> create_mapper(Cartridge &cart);

#endif
