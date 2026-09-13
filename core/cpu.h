#ifndef CPU_H
#define CPU_H

#include "bus.h"

class CPU {
  Bus &bus_;

  uint8_t a_ = 0;
  uint8_t x_ = 0;
  uint8_t y_ = 0;
  uint8_t p_ = 0;
  uint8_t sp_ = 0;
  uint16_t pc_ = 0;

  void set_zn_(uint8_t value);

public:
  bool halt = false;
  explicit CPU(Bus &b) : bus_(b) {}

  uint8_t a() const { return a_; }
  uint8_t x() const { return x_; }
  uint8_t y() const { return y_; }
  uint8_t p() const { return p_; }
  uint8_t sp() const { return sp_; }
  uint16_t pc() const { return pc_; }

  void set_pc(uint16_t addr) { pc_ = addr; }

  void reset();
  void step();
};

#endif