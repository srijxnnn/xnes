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
  /* ======== ACCESS ======== */

  // LDA #Immediate
  case 0xA9: {
    const uint8_t op = bus_.read(pc_++);
    a_ = op;
    set_zn_(a_);
    break;
  }

  // LDA Zero Page
  case 0xA5: {
    const uint8_t zp = bus_.read(pc_++);
    a_ = bus_.read(zp);
    set_zn_(a_);
    break;
  }

  // LDA Zero Page,X
  case 0xB5: {
    const uint8_t zp = bus_.read(pc_++);
    a_ = bus_.read(static_cast<uint8_t>(zp + x_));
    set_zn_(a_);
    break;
  }

  // LDA Absolute
  case 0xAD: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    a_ =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    set_zn_(a_);
    break;
  }

  // LDA Absolute,X
  case 0xBD: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    a_ = bus_.read(base + x_);
    set_zn_(a_);
    break;
  }

  // LDA Absolute,Y
  case 0xB9: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    a_ = bus_.read(base + y_);
    set_zn_(a_);
    break;
  }

  // LDA (Indirect,X)
  case 0xA1: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + x_ + 1));
    a_ =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    set_zn_(a_);
    break;
  }

  // LDA (Indirect),Y
  case 0xB1: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + 1));
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    a_ = bus_.read(base + y_);
    set_zn_(a_);
    break;
  }

  // STA Zero Page
  case 0x85: {
    const uint8_t zp = bus_.read(pc_++);
    bus_.write(zp, a_);
    break;
  }

  // STA Zero Page
  case 0x95: {
    const uint8_t zp = bus_.read(pc_++);
    bus_.write(static_cast<uint8_t>(zp + x_), a_);
    break;
  }

  // STA Absolute
  case 0x8D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    bus_.write((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo),
               a_);
    break;
  }

  // STA Absolute,X
  case 0x9D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    bus_.write(base + x_, a_);
    break;
  }

  // STA Absolute,Y
  case 0x99: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    bus_.write(base + y_, a_);
    break;
  }

  // STA (Indirect,X)
  case 0x81: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + x_ + 1));
    bus_.write((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo),
               a_);
    break;
  }

  // STA (Indirect),Y
  case 0x91: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + 1));
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    bus_.write(base + y_, a_);
    break;
  }

  // LDX #Immediate
  case 0xA2: {
    const uint8_t op = bus_.read(pc_++);
    x_ = op;
    set_zn_(x_);
    break;
  }

  // LDX Zero Page
  case 0xA6: {
    const uint8_t zp = bus_.read(pc_++);
    x_ = bus_.read(zp);
    set_zn_(x_);
    break;
  }

  // LDX Zero Page,Y
  case 0xB6: {
    const uint8_t zp = bus_.read(pc_++);
    x_ = bus_.read(static_cast<uint8_t>(zp + y_));
    set_zn_(x_);
    break;
  }

  // LDX Absolute
  case 0xAE: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    x_ =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    set_zn_(x_);
    break;
  }

  // LDX Absolute,Y
  case 0xBE: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    x_ = bus_.read(base + y_);
    set_zn_(x_);
    break;
  }

  // STX Zero Page
  case 0x86: {
    const uint8_t zp = bus_.read(pc_++);
    bus_.write(zp, x_);
    break;
  }

  // STX Zero Page,Y
  case 0x96: {
    const uint8_t zp = bus_.read(pc_++);
    bus_.write(static_cast<uint8_t>(zp + y_), x_);
    break;
  }

  // STX Absolute
  case 0x8E: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    bus_.write((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo),
               x_);
    break;
  }

  // LDY #Immediate
  case 0xA0: {
    const uint8_t op = bus_.read(pc_++);
    y_ = op;
    set_zn_(y_);
    break;
  }

  // LDY Zero Page
  case 0xA4: {
    const uint8_t zp = bus_.read(pc_++);
    y_ = bus_.read(zp);
    set_zn_(y_);
    break;
  }

  // LDY Zero Page,X
  case 0xB4: {
    const uint8_t zp = bus_.read(pc_++);
    y_ = bus_.read(static_cast<uint8_t>(zp + x_));
    set_zn_(y_);
    break;
  }

  // LDY Absolute
  case 0xAC: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    y_ =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    set_zn_(y_);
    break;
  }

  // LDY Absolute,X
  case 0xBC: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    y_ = bus_.read(base + x_);
    set_zn_(y_);
    break;
  }

  // STY Zero Page
  case 0x84: {
    const uint8_t zp = bus_.read(pc_++);
    bus_.write(zp, y_);
    break;
  }

  // STY Zero Page,X
  case 0x94: {
    const uint8_t zp = bus_.read(pc_++);
    bus_.write(static_cast<uint8_t>(zp + x_), y_);
    break;
  }

  // STY Absolute
  case 0x8C: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    bus_.write((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo),
               y_);
    break;
  }

  /* ======== TRANSFER ======== */

  // TAX
  case 0xAA: {
    x_ = a_;
    set_zn_(x_);
    break;
  }

  // TXA
  case 0x8A: {
    a_ = x_;
    set_zn_(a_);
    break;
  }

  // TAY
  case 0xA8: {
    y_ = a_;
    set_zn_(y_);
    break;
  }

  // TYA
  case 0x98: {
    a_ = y_;
    set_zn_(a_);
    break;
  }

  /* ======== ARITHMETIC ======== */

  // ADC #Immediate
  case 0x69: {
    const uint8_t op = bus_.read(pc_++);
    const uint16_t result = a_ + op + (p_ & 0x01);
    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // ADC Zero Page
  case 0x65: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(zp);
    const uint16_t result = a_ + op + (p_ & 0x01);

    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // ADC Zero Page,X
  case 0x75: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint16_t result = a_ + op + (p_ & 0x01);

    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // ADC Absolute
  case 0x6D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint8_t op =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    const uint16_t result = a_ + op + (p_ & 0x01);

    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // ADC Absolute,X
  case 0x7D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + x_);
    const uint16_t result = a_ + op + (p_ & 0x01);

    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // ADC Absolute,Y
  case 0x79: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + y_);
    const uint16_t result = a_ + op + (p_ & 0x01);

    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // ADC (Indirect,X)
  case 0x61: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + x_ + 1));
    const uint8_t op =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    const uint16_t result = a_ + op + (p_ & 0x01);

    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // ADC (Indirect),Y
  case 0x71: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + 1));
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + y_);
    const uint16_t result = a_ + op + (p_ & 0x01);

    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // SBC #Immediate
  case 0xE9: {
    const uint8_t op = ~bus_.read(pc_++);
    const uint16_t result = a_ + op + (p_ & 0x01);
    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // SBC Zero Page
  case 0xE5: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = ~bus_.read(zp);
    const uint16_t result = a_ + op + (p_ & 0x01);
    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // SBC Zero Page,X
  case 0xF5: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = ~bus_.read(static_cast<uint8_t>(zp + x_));
    const uint16_t result = a_ + op + (p_ & 0x01);
    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // SBC Absolute
  case 0xED: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint8_t op = ~bus_.read((static_cast<uint16_t>(hi) << 8) |
                                  static_cast<uint16_t>(lo));
    const uint16_t result = a_ + op + (p_ & 0x01);
    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // SBC Absolute,X
  case 0xFD: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = ~bus_.read(base + x_);
    const uint16_t result = a_ + op + (p_ & 0x01);
    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // SBC Absolute,Y
  case 0xF9: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = ~bus_.read(base + y_);
    const uint16_t result = a_ + op + (p_ & 0x01);
    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // SBC (Indirect,X)
  case 0xE1: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + x_ + 1));
    const uint8_t op = ~bus_.read((static_cast<uint16_t>(hi) << 8) |
                                  static_cast<uint16_t>(lo));
    const uint16_t result = a_ + op + (p_ & 0x01);
    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // SBC (Indirect),Y
  case 0xF1: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + 1));
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = ~bus_.read(base + y_);
    const uint16_t result = a_ + op + (p_ & 0x01);
    if (result > 0xFF) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    p_ &= ~0x40;
    p_ |= ((a_ ^ static_cast<uint8_t>(result)) &
           (op ^ static_cast<uint8_t>(result)) & 0x80) >>
          1;
    a_ = static_cast<uint8_t>(result);
    set_zn_(a_);
    break;
  }

  // INC Zero Page
  case 0xE6: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(zp);
    op++;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // INC Zero Page,X
  case 0xF6: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    op++;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // INC Absolute
  case 0xEE: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr);
    op++;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // INC Absolute,X
  case 0xFE: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr + x_);
    op++;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // DEC Zero Page
  case 0xC6: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(zp);
    op--;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // DEC Zero Page,X
  case 0xD6: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    op--;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // DEC Absolute
  case 0xCE: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr);
    op--;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // DEC Absolute,X
  case 0xDE: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr + x_);
    op--;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // INX
  case 0xE8: {
    x_++;
    set_zn_(x_);
    break;
  }

  // DEX
  case 0xCA: {
    x_--;
    set_zn_(x_);
    break;
  }

  // INY
  case 0xC8: {
    y_++;
    set_zn_(y_);
    break;
  }

  // DEY
  case 0x88: {
    y_--;
    set_zn_(y_);
    break;
  }

  /* ======== SHIFT ======== */

  // ASL Accumulator
  case 0x0A: {
    p_ &= ~(p_ & 0x01);
    p_ |= ((a_ & 0x80) >> 7);
    a_ <<= 1;
    set_zn_(a_);
    break;
  }

  // ASL Zero Page
  case 0x06: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(zp);
    p_ &= ~(p_ & 0x01);
    p_ |= ((op & 0x80) >> 7);
    op <<= 1;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // ASL Zero Page,X
  case 0x16: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    p_ &= ~(p_ & 0x01);
    p_ |= ((op & 0x80) >> 7);
    op <<= 1;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // ASL Absolute
  case 0x0E: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr);
    p_ &= ~(p_ & 0x01);
    p_ |= ((op & 0x80) >> 7);
    op <<= 1;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // ASL Absolute
  case 0x1E: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr + x_);
    p_ &= ~(p_ & 0x01);
    p_ |= ((op & 0x80) >> 7);
    op <<= 1;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // LSR Accumulator
  case 0x4A: {
    p_ &= ~(p_ & 0x01);
    p_ |= (a_ & 0x01);
    a_ >>= 1;
    set_zn_(a_);
    break;
  }

  // LSR Zero Page
  case 0x46: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(zp);
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x01);
    op >>= 1;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // LSR Zero Page,X
  case 0x56: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x01);
    op >>= 1;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // LSR Absolute
  case 0x4E: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr);
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x01);
    op >>= 1;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // LSR Absolute,X
  case 0x5E: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr + x_);
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x01);
    op >>= 1;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // ROL Accumulator
  case 0x2A: {
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (a_ & 0x80) >> 7;
    a_ <<= 1;
    a_ |= old_p & 0x01;
    set_zn_(a_);
    break;
  }

  // ROL Zero Page
  case 0x26: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(zp);
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x80) >> 7;
    op <<= 1;
    op |= old_p & 0x01;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // ROL Zero Page,X
  case 0x36: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x80) >> 7;
    op <<= 1;
    op |= old_p & 0x01;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // ROL Absolute
  case 0x2E: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr);
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x80) >> 7;
    op <<= 1;
    op |= old_p & 0x01;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // ROL Absolute,X
  case 0x3E: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr + x_);
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x80) >> 7;
    op <<= 1;
    op |= old_p & 0x01;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // ROR Accumulator
  case 0x6A: {
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (a_ & 0x01);
    a_ >>= 1;
    a_ |= (old_p & 0x01) << 7;
    set_zn_(a_);
    break;
  }

  // ROR Zero Page
  case 0x66: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(zp);
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x01);
    op >>= 1;
    op |= (old_p & 0x01) << 7;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // ROR Zero Page,X
  case 0x76: {
    const uint8_t zp = bus_.read(pc_++);
    uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x01);
    op >>= 1;
    op |= (old_p & 0x01) << 7;
    bus_.write(zp, op);
    set_zn_(op);
    break;
  }

  // ROR Absolute
  case 0x6E: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr);
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x01);
    op >>= 1;
    op |= (old_p & 0x01) << 7;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  // ROR Absolute,X
  case 0x7E: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t addr =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    uint8_t op = bus_.read(addr + x_);
    const uint8_t old_p = p_;
    p_ &= ~(p_ & 0x01);
    p_ |= (op & 0x01);
    op >>= 1;
    op |= (old_p & 0x01) << 7;
    bus_.write(addr, op);
    set_zn_(op);
    break;
  }

  /* ======== BITWISE ======== */

  // AND #Immediate
  case 0x29: {
    const uint8_t op = bus_.read(pc_++);
    a_ &= op;
    set_zn_(a_);
    break;
  }

  // AND Zero Page
  case 0x25: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(zp);
    a_ &= op;
    set_zn_(a_);
    break;
  }

  // AND Zero Page,X
  case 0x35: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    a_ &= op;
    set_zn_(a_);
    break;
  }

  // AND Absolute
  case 0x2D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    a_ &=
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    set_zn_(a_);
    break;
  }

  // AND Absolute,X
  case 0x3D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + x_);
    a_ &= op;
    set_zn_(a_);
    break;
  }

  // AND Absolute,Y
  case 0x39: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + y_);
    a_ &= op;
    set_zn_(a_);
    break;
  }

  // AND (Indirect,X)
  case 0x21: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + x_ + 1));
    a_ &=
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    set_zn_(a_);
    break;
  }

  // AND (Indirect),Y
  case 0x31: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + 1));
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    a_ &= bus_.read(base + y_);
    set_zn_(a_);
    break;
  }

  // ORA #Immediate
  case 0x09: {
    const uint8_t op = bus_.read(pc_++);
    a_ |= op;
    set_zn_(a_);
    break;
  }

  // ORA Zero Page
  case 0x05: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(zp);
    a_ |= op;
    set_zn_(a_);
    break;
  }

  // ORA Zero Page,X
  case 0x15: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    a_ |= op;
    set_zn_(a_);
    break;
  }

  // ORA Absolute
  case 0x0D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint8_t op =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    a_ |= op;
    set_zn_(a_);
    break;
  }

  // ORA Absolute,X
  case 0x1D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + x_);
    a_ |= op;
    set_zn_(a_);
    break;
  }

  // ORA Absolute,Y
  case 0x19: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + y_);
    a_ |= op;
    set_zn_(a_);
    break;
  }

  // ORA (Indirect,X)
  case 0x01: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + x_ + 1));
    a_ |=
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    set_zn_(a_);
    break;
  }

  // ORA (Indirect),Y
  case 0x11: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + 1));
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    a_ |= bus_.read(base + y_);
    set_zn_(a_);
    break;
  }

  // EOR #Immediate
  case 0x49: {
    const uint8_t op = bus_.read(pc_++);
    a_ ^= op;
    set_zn_(a_);
    break;
  }

  // EOR Zero Page
  case 0x45: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(zp);
    a_ ^= op;
    set_zn_(a_);
    break;
  }

  // EOR Zero Page,X
  case 0x55: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    a_ ^= op;
    set_zn_(a_);
    break;
  }

  // EOR Absolute
  case 0x4D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    a_ ^=
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    set_zn_(a_);
    break;
  }

  // EOR Absolute,X
  case 0x5D: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + x_);
    a_ ^= op;
    set_zn_(a_);
    break;
  }

  // EOR Absolute,Y
  case 0x59: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + y_);
    a_ ^= op;
    set_zn_(a_);
    break;
  }

  // EOR (Indirect,X)
  case 0x41: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + x_ + 1));
    a_ ^=
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    set_zn_(a_);
    break;
  }

  // EOR (Indirect),Y
  case 0x51: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + 1));
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    a_ ^= bus_.read(base + y_);
    set_zn_(a_);
    break;
  }

  // BIT Zero Page
  case 0x24: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t mem = bus_.read(zp);
    const uint8_t result = a_ & mem;
    p_ &= ~0x82;
    if (result == 0) {
      p_ |= 0x02;
    }
    p_ &= ~0xC0;
    p_ |= mem & 0xC0;
    break;
  }

  // BIT Absolute
  case 0x2C: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint8_t mem =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    const uint8_t result = a_ & mem;
    p_ &= ~0x82;
    if (result == 0) {
      p_ |= 0x02;
    }
    p_ &= ~0xC0;
    p_ |= mem & 0xC0;
    break;
  }

  /* ======== COMPARE ======== */

  // CMP #Immediate
  case 0xC9: {
    const uint8_t op = bus_.read(pc_++);
    const uint8_t result = a_ - op;
    if (a_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CMP Zero Page
  case 0xC5: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(zp);
    const uint8_t result = a_ - op;
    if (a_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CMP Zero Page,X
  case 0xD5: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t result = a_ - op;
    if (a_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CMP Absolute
  case 0xCD: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint8_t op =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    const uint8_t result = a_ - op;
    if (a_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CMP Absolute,X
  case 0xDD: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + x_);
    const uint8_t result = a_ - op;
    if (a_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CMP Absolute,Y
  case 0xD9: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + y_);
    const uint8_t result = a_ - op;
    if (a_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CMP (Indirect,X)
  case 0xC1: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp + x_));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + x_ + 1));
    const uint8_t op =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    const uint8_t result = a_ - op;
    if (a_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CMP (Indirect),Y
  case 0xD1: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t lo = bus_.read(static_cast<uint8_t>(zp));
    const uint8_t hi = bus_.read(static_cast<uint8_t>(zp + 1));
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    const uint8_t op = bus_.read(base + y_);
    const uint8_t result = a_ - op;
    if (a_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CPX #Immediate
  case 0xE0: {
    const uint8_t op = bus_.read(pc_++);
    const uint8_t result = x_ - op;
    if (x_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CPX Zero Page
  case 0xE4: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(zp);
    const uint8_t result = x_ - op;
    if (x_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CPX Absolute
  case 0xEC: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint8_t op =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    const uint8_t result = x_ - op;
    if (x_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CPY #Immediate
  case 0xC0: {
    const uint8_t op = bus_.read(pc_++);
    const uint8_t result = y_ - op;
    if (y_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CPY Zero Page
  case 0xC4: {
    const uint8_t zp = bus_.read(pc_++);
    const uint8_t op = bus_.read(zp);
    const uint8_t result = y_ - op;
    if (y_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  // CPY Absolute
  case 0xCC: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    const uint8_t op =
        bus_.read((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    const uint8_t result = y_ - op;
    if (y_ >= op) {
      p_ |= 0x01;
    } else {
      p_ &= ~0x01;
    }
    set_zn_(result);
    break;
  }

  /* ======== BRANCH ======== */

  // BCC
  case 0x90: {
    const uint8_t offset = bus_.read(pc_++);
    if ((p_ & 0x01) == 0) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }

  // BCS
  case 0xB0: {
    const uint8_t offset = bus_.read(pc_++);
    if (p_ & 0x01) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }

  // BEQ
  case 0xF0: {
    const uint8_t offset = bus_.read(pc_++);
    if (p_ & 0x02) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }

  // BNE
  case 0xD0: {
    const uint8_t offset = bus_.read(pc_++);
    if ((p_ & 0x02) == 0) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }

  // BPL
  case 0x10: {
    const uint8_t offset = bus_.read(pc_++);
    if ((p_ & 0x80) == 0) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }

  // BMI
  case 0x30: {
    const uint8_t offset = bus_.read(pc_++);
    if (p_ & 0x80) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }

  // BVC
  case 0x50: {
    const uint8_t offset = bus_.read(pc_++);
    if ((p_ & 0x40) == 0) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }

  // BVS
  case 0x70: {
    const uint8_t offset = bus_.read(pc_++);
    if (p_ & 0x40) {
      pc_ = pc_ + static_cast<int8_t>(offset);
    }
    break;
  }

  /* ======== JUMP ======== */

  // JMP Absolute
  case 0x4C: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    break;
  }

  // JMP (Indirect)
  case 0x6C: {
    uint8_t lo = bus_.read(pc_++);
    uint8_t hi = bus_.read(pc_++);
    const uint16_t base =
        (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    lo = bus_.read(base);
    hi = bus_.read((base & 0xFF00) + static_cast<uint8_t>(base + 1));
    pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    break;
  }

  // JSR
  case 0x20: {
    const uint8_t lo = bus_.read(pc_++);
    const uint8_t hi = bus_.read(pc_++);
    bus_.write(0x0100 | sp_--, ((pc_ - 1) >> 8));
    bus_.write(0x0100 | sp_--, (pc_ - 1) & 0xFF);
    pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    break;
  }

  // RTS
  case 0x60: {
    const uint8_t lo = bus_.read(0x0100 | ++sp_);
    const uint8_t hi = bus_.read(0x0100 | ++sp_);
    pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    pc_++;
    break;
  }

  // RTI
  case 0x40: {
    const uint8_t status = bus_.read(0x0100 | ++sp_);
    p_ = (status & 0xCF) | (p_ & (~0xCF));

    const uint8_t lo = bus_.read(0x0100 | ++sp_);
    const uint8_t hi = bus_.read(0x0100 | ++sp_);
    pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);

    break;
  }

  /* ======== STACK ======== */

  // PHA
  case 0x48: {
    bus_.write(0x0100 | sp_--, a_);
    break;
  }

  // PLA
  case 0x68: {
    const uint8_t val = bus_.read(0x0100 | ++sp_);
    a_ = val;
    set_zn_(a_);
    break;
  }

  // PHP
  case 0x08: {
    bus_.write(0x0100 | sp_--, p_ | 0x10);
    break;
  }

  // PLP
  case 0x28: {
    const uint8_t status = bus_.read(0x0100 | ++sp_);
    p_ = (status & 0xCF) | (p_ & (~0xCF));
    break;
  }

  // TXS
  case 0x9A: {
    sp_ = x_;
    break;
  }

  // TSX
  case 0xBA: {
    x_ = sp_;
    set_zn_(x_);
    break;
  }

  /* ======== FLAGS ======== */

  // CLC
  case 0x18: {
    p_ &= ~0x01;
    break;
  }

  // SEC
  case 0x38: {
    p_ |= 0x01;
    break;
  }

  // SEI
  case 0x78: {
    p_ |= 0x04;
    break;
  }

  // CLD
  case 0xD8: {
    p_ &= ~0x08;
    break;
  }

  // SED
  case 0xF8: {
    p_ |= 0x08;
    break;
  }

  // CLV
  case 0xB8: {
    p_ &= ~0x40;
    break;
  }

  /* ======== OTHER ======== */

  // NOP
  case 0xEA: {
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