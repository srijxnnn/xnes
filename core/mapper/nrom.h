#ifndef NROM_H
#define NROM_H

#include "mapper/mapper.h"

// iNES mapper 0: no banking at all. PRG is mirrored across $8000-$FFFF, so a
// 16KB game sees the same bank at $8000 and $C000, and 8KB of work RAM sits at
// $6000. Donkey Kong and nestest are both this board.
class NROM : public Mapper {
public:
  using Mapper::Mapper;

  uint8_t cpu_read(uint16_t addr) const override;
  void cpu_write(uint16_t addr, uint8_t data) override;

  uint8_t chr_read(uint16_t addr) const override;
  void chr_write(uint16_t addr, uint8_t data) override;
};

#endif
