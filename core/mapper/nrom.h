#ifndef NROM_H
#define NROM_H

#include "mapper/mapper.h"

class NROM : public Mapper {
public:
  using Mapper::Mapper;

  uint8_t cpu_read(uint16_t addr) const override;
  void cpu_write(uint16_t addr, uint8_t data) override;

  uint8_t chr_read(uint16_t addr) const override;
  void chr_write(uint16_t addr, uint8_t data) override;
};

#endif
