#include "mapper.h"
#include "nrom.h"

std::unique_ptr<Mapper> create_mapper(Cartridge &cart) {
  switch (cart.mapper_id()) {
  case 0:
    return std::make_unique<NROM>(cart);
  default:
    return nullptr;
  }
}
