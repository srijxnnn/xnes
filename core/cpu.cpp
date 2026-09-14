#include "cpu.h"
#include <cstdint>

void CPU::reset() {
  a_ = 0;
  x_ = 0;
  y_ = 0;
  p_ = 0x24;
  sp_ -= 3;

  const uint8_t lo = bus_.read(0xFFFC);
  const uint8_t hi = bus_.read(0xFFFD);

  pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
}

void CPU::step() {
  const uint8_t opcode = bus_.read(pc_++);

  switch (opcode) {
  case 0x08: {
    bus_.write(0x0100 | sp_--, p_ | 0x10);
    break;
  }
  case 0x10: {
    const uint8_t offset = bus_.read(pc_++);
    if ((p_ & 0x80) == 0) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
    break;
  }
  case 0x18: {
    p_ &= ~0x01;
    break;
  }
  case 0x20: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    bus_.write(0x0100 | sp_--, ((pc_ - 1) >> 8));
    bus_.write(0x0100 | sp_--, (pc_ - 1) & 0xFF);
    pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    break;
  }
  case 0x24: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t mem = bus_.read(static_cast<uint16_t>(zp));
    const uint8_t result = a_ & mem;
    p_ &= ~0x82;
    if (result == 0) {
      p_ |= 0x02;
    }
    p_ &= ~0xC0;
    p_ |= mem & 0xC0;
    break;
  }
  case 0x28: {
    const uint8_t status = bus_.read(0x0100 | ++sp_);
    p_ = (status & 0xCF) | (p_ & (~0xCF));
    break;
  }
  case 0x29: {
    const uint8_t op = bus_.read(pc_++);
    a_ &= op;
    set_zn_(a_);
    break;
  }
  case 0x38: {
    p_ |= 0x01;
    break;
  }
  case 0x48: {
    bus_.write(0x0100 | sp_--, a_);
    break;
  }
  case 0x4C: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    break;
  }
  case 0x50: {
    const uint8_t offset = bus_.read(pc_++);
    if ((p_ & 0x40) == 0) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }
  case 0x60: {
    const uint8_t lo = bus_.read(0x0100 | ++sp_);
    const uint8_t hi = bus_.read(0x0100 | ++sp_);
    pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    pc_++;
    break;
  }
  case 0x68: {
    const uint8_t val = bus_.read(0x0100 | ++sp_);
    a_ = val;
    set_zn_(a_);
    break;
  }
  case 0x70: {
    const uint8_t offset = bus_.read(pc_++);
    if (p_ & 0x40) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }
  case 0x78: {
    p_ |= 0x04;
    break;
  }
  case 0x85: {
    const uint8_t zp = bus_.read(pc_++);
    bus_.write(static_cast<uint16_t>(zp), a_);
    break;
  }
  case 0x86: {
    const uint8_t zp = bus_.read(pc_++);
    bus_.write(static_cast<uint16_t>(zp), x_);
    break;
  }
  case 0x90: {
    const uint8_t offset = bus_.read(pc_++);
    if ((p_ & 0x01) == 0) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }
  case 0xA2: {
    const uint8_t op = bus_.read(pc_++);
    x_ = op;
    set_zn_(x_);
    break;
  }
  case 0xA9: {
    const uint8_t op = bus_.read(pc_++);
    a_ = op;
    set_zn_(a_);
    break;
  }
  case 0xB0: {
    const uint8_t offset = bus_.read(pc_++);
    if (p_ & 0x01) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }
  case 0xC9: {
    const uint8_t op = bus_.read(pc_++);
    const uint8_t result = a_ - op;
    if (result >= 0) {
      p_ |= 0x01;
    }
    set_zn_(result);
    break;
  }
  case 0xD0: {
    const uint8_t offset = bus_.read(pc_++);
    if ((p_ & 0x02) == 0) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }
  case 0xD8: {
    p_ &= ~0x08;
    break;
  }
  case 0xEA: {
    break;
  }
  case 0xF0: {
    const uint8_t offset = bus_.read(pc_++);
    if (p_ & 0x02) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }
  case 0xF8: {
    p_ |= 0x08;
    break;
  }
  default: {
    halt = true;
  }
  }

  return;
}

void CPU::set_zn_(uint8_t value) {
  p_ &= ~0x82;
  if (value == 0) {
    p_ |= 0x02;
  }
  if (value & 0x80) {
    p_ |= 0x80;
  }
}