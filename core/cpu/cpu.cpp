#include "cpu.h"
#include <cstdint>

void CPU::reset() {
  a_ = 0;
  x_ = 0;
  y_ = 0;
  p_ = Interrupt | Unused;
  sp_ = 0xFD;
  pc_ = read16_(0xFFFC);
  cycles_ = 7;
  halted_ = false;
  nmi_pending_ = false;
}

int CPU::step() {
  const uint64_t start = cycles_;

  if (halted_) {
    cycles_ += 2;
    return 2;
  }

  if (nmi_pending_) {
    nmi_pending_ = false;
    take_nmi_();
  } else {
    const uint8_t opcode = bus_.read(pc_++);
    const Instruction &in = table_[opcode];

    page_crossed_ = false;
    const uint16_t addr = resolve_(in.mode);

    cycles_ += in.cycles;
    if (in.page_penalty && page_crossed_) {
      cycles_++;
    }

    // Taken branches bill their own extra cycles from inside the handler.
    (this->*in.exec)(addr);
  }

  cycles_ += bus_.drain_stall();
  return static_cast<int>(cycles_ - start);
}

void CPU::take_nmi_() {
  push_(pc_ >> 8);
  push_(pc_ & 0xFF);
  push_((p_ & ~Break) | Unused);
  p_ |= Interrupt;
  pc_ = read16_(0xFFFA);
  cycles_ += 7;
}

/* ======== ADDRESSING ======== */

uint16_t CPU::read16_(uint16_t addr) {
  const uint8_t lo = bus_.read(addr);
  const uint8_t hi = bus_.read(addr + 1);
  return (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
}

// Pointers held in zero page wrap inside it: reading a pointer at $FF takes
// its high byte from $00, not $0100.
uint16_t CPU::read16_zp_(uint8_t addr) {
  const uint8_t lo = bus_.read(addr);
  const uint8_t hi = bus_.read(static_cast<uint8_t>(addr + 1));
  return (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
}

uint16_t CPU::resolve_(Mode mode) {
  switch (mode) {
  case Mode::Implied:
  case Mode::Accumulator:
    return 0;

  case Mode::Immediate:
    return pc_++;

  case Mode::ZeroPage:
    return bus_.read(pc_++);

  case Mode::ZeroPageX:
    return static_cast<uint8_t>(bus_.read(pc_++) + x_);

  case Mode::ZeroPageY:
    return static_cast<uint8_t>(bus_.read(pc_++) + y_);

  case Mode::Absolute: {
    const uint16_t addr = read16_(pc_);
    pc_ += 2;
    return addr;
  }

  case Mode::AbsoluteX: {
    const uint16_t base = read16_(pc_);
    pc_ += 2;
    const uint16_t addr = base + x_;
    page_crossed_ = (base & 0xFF00) != (addr & 0xFF00);
    return addr;
  }

  case Mode::AbsoluteY: {
    const uint16_t base = read16_(pc_);
    pc_ += 2;
    const uint16_t addr = base + y_;
    page_crossed_ = (base & 0xFF00) != (addr & 0xFF00);
    return addr;
  }

  // Only used by JMP, and only on hardware that forgets to carry into the
  // high byte: JMP ($10FF) reads its target from $10FF and $1000.
  case Mode::Indirect: {
    const uint16_t ptr = read16_(pc_);
    pc_ += 2;
    const uint8_t lo = bus_.read(ptr);
    const uint8_t hi =
        bus_.read((ptr & 0xFF00) | static_cast<uint8_t>(ptr + 1));
    return (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
  }

  case Mode::IndirectX:
    return read16_zp_(static_cast<uint8_t>(bus_.read(pc_++) + x_));

  case Mode::IndirectY: {
    const uint16_t base = read16_zp_(bus_.read(pc_++));
    const uint16_t addr = base + y_;
    page_crossed_ = (base & 0xFF00) != (addr & 0xFF00);
    return addr;
  }

  case Mode::Relative: {
    const int8_t offset = static_cast<int8_t>(bus_.read(pc_++));
    return pc_ + offset;
  }
  }

  return 0;
}

/* ======== PRIMITIVES ======== */

void CPU::push_(uint8_t value) { bus_.write(0x0100 | sp_--, value); }

uint8_t CPU::pull_() { return bus_.read(0x0100 | ++sp_); }

void CPU::set_flag_(uint8_t mask, bool value) {
  if (value) {
    p_ |= mask;
  } else {
    p_ &= ~mask;
  }
}

void CPU::set_zn_(uint8_t value) {
  set_flag_(Zero, value == 0);
  set_flag_(Negative, value & 0x80);
}

void CPU::compare_(uint8_t reg, uint16_t addr) {
  const uint8_t op = bus_.read(addr);
  set_flag_(Carry, reg >= op);
  set_zn_(static_cast<uint8_t>(reg - op));
}

void CPU::branch_(bool condition, uint16_t target) {
  if (!condition) {
    return;
  }

  cycles_ += (pc_ & 0xFF00) == (target & 0xFF00) ? 1 : 2;
  pc_ = target;
}

// SBC is ADC of the one's complement, so both share this.
void CPU::add_(uint8_t value) {
  const uint16_t result = a_ + value + (p_ & Carry);
  set_flag_(Carry, result > 0xFF);
  set_flag_(Overflow, (a_ ^ result) & (value ^ result) & 0x80);
  a_ = static_cast<uint8_t>(result);
  set_zn_(a_);
}

uint8_t CPU::shift_left_(uint8_t value) {
  set_flag_(Carry, value & 0x80);
  return value << 1;
}

uint8_t CPU::shift_right_(uint8_t value) {
  set_flag_(Carry, value & 0x01);
  return value >> 1;
}

uint8_t CPU::rotate_left_(uint8_t value) {
  const uint8_t carry = p_ & Carry;
  set_flag_(Carry, value & 0x80);
  return (value << 1) | carry;
}

uint8_t CPU::rotate_right_(uint8_t value) {
  const uint8_t carry = p_ & Carry;
  set_flag_(Carry, value & 0x01);
  return (value >> 1) | (carry << 7);
}

/* ======== ACCESS ======== */

void CPU::lda_(uint16_t addr) {
  a_ = bus_.read(addr);
  set_zn_(a_);
}

void CPU::ldx_(uint16_t addr) {
  x_ = bus_.read(addr);
  set_zn_(x_);
}

void CPU::ldy_(uint16_t addr) {
  y_ = bus_.read(addr);
  set_zn_(y_);
}

void CPU::sta_(uint16_t addr) { bus_.write(addr, a_); }

void CPU::stx_(uint16_t addr) { bus_.write(addr, x_); }

void CPU::sty_(uint16_t addr) { bus_.write(addr, y_); }

/* ======== TRANSFER ======== */

void CPU::tax_(uint16_t) {
  x_ = a_;
  set_zn_(x_);
}

void CPU::tay_(uint16_t) {
  y_ = a_;
  set_zn_(y_);
}

void CPU::txa_(uint16_t) {
  a_ = x_;
  set_zn_(a_);
}

void CPU::tya_(uint16_t) {
  a_ = y_;
  set_zn_(a_);
}

void CPU::tsx_(uint16_t) {
  x_ = sp_;
  set_zn_(x_);
}

void CPU::txs_(uint16_t) { sp_ = x_; }

/* ======== ARITHMETIC ======== */

void CPU::adc_(uint16_t addr) { add_(bus_.read(addr)); }

void CPU::sbc_(uint16_t addr) { add_(~bus_.read(addr)); }

void CPU::inc_(uint16_t addr) {
  const uint8_t value = bus_.read(addr) + 1;
  bus_.write(addr, value);
  set_zn_(value);
}

void CPU::dec_(uint16_t addr) {
  const uint8_t value = bus_.read(addr) - 1;
  bus_.write(addr, value);
  set_zn_(value);
}

void CPU::inx_(uint16_t) { set_zn_(++x_); }

void CPU::iny_(uint16_t) { set_zn_(++y_); }

void CPU::dex_(uint16_t) { set_zn_(--x_); }

void CPU::dey_(uint16_t) { set_zn_(--y_); }

/* ======== SHIFT ======== */

void CPU::asl_(uint16_t addr) {
  const uint8_t value = shift_left_(bus_.read(addr));
  bus_.write(addr, value);
  set_zn_(value);
}

void CPU::asl_a_(uint16_t) {
  a_ = shift_left_(a_);
  set_zn_(a_);
}

void CPU::lsr_(uint16_t addr) {
  const uint8_t value = shift_right_(bus_.read(addr));
  bus_.write(addr, value);
  set_zn_(value);
}

void CPU::lsr_a_(uint16_t) {
  a_ = shift_right_(a_);
  set_zn_(a_);
}

void CPU::rol_(uint16_t addr) {
  const uint8_t value = rotate_left_(bus_.read(addr));
  bus_.write(addr, value);
  set_zn_(value);
}

void CPU::rol_a_(uint16_t) {
  a_ = rotate_left_(a_);
  set_zn_(a_);
}

void CPU::ror_(uint16_t addr) {
  const uint8_t value = rotate_right_(bus_.read(addr));
  bus_.write(addr, value);
  set_zn_(value);
}

void CPU::ror_a_(uint16_t) {
  a_ = rotate_right_(a_);
  set_zn_(a_);
}

/* ======== BITWISE ======== */

void CPU::and_(uint16_t addr) {
  a_ &= bus_.read(addr);
  set_zn_(a_);
}

void CPU::ora_(uint16_t addr) {
  a_ |= bus_.read(addr);
  set_zn_(a_);
}

void CPU::eor_(uint16_t addr) {
  a_ ^= bus_.read(addr);
  set_zn_(a_);
}

void CPU::bit_(uint16_t addr) {
  const uint8_t mem = bus_.read(addr);
  set_flag_(Zero, (a_ & mem) == 0);
  set_flag_(Negative, mem & 0x80);
  set_flag_(Overflow, mem & 0x40);
}

/* ======== COMPARE ======== */

void CPU::cmp_(uint16_t addr) { compare_(a_, addr); }

void CPU::cpx_(uint16_t addr) { compare_(x_, addr); }

void CPU::cpy_(uint16_t addr) { compare_(y_, addr); }

/* ======== BRANCH ======== */

void CPU::bcc_(uint16_t addr) { branch_((p_ & Carry) == 0, addr); }

void CPU::bcs_(uint16_t addr) { branch_(p_ & Carry, addr); }

void CPU::beq_(uint16_t addr) { branch_(p_ & Zero, addr); }

void CPU::bne_(uint16_t addr) { branch_((p_ & Zero) == 0, addr); }

void CPU::bpl_(uint16_t addr) { branch_((p_ & Negative) == 0, addr); }

void CPU::bmi_(uint16_t addr) { branch_(p_ & Negative, addr); }

void CPU::bvc_(uint16_t addr) { branch_((p_ & Overflow) == 0, addr); }

void CPU::bvs_(uint16_t addr) { branch_(p_ & Overflow, addr); }

/* ======== JUMP ======== */

void CPU::jmp_(uint16_t addr) { pc_ = addr; }

void CPU::jsr_(uint16_t addr) {
  // resolve_ already consumed both operand bytes, so pc_ - 1 is the last byte
  // of this instruction, which is what the 6502 pushes.
  const uint16_t ret = pc_ - 1;
  push_(ret >> 8);
  push_(ret & 0xFF);
  pc_ = addr;
}

void CPU::rts_(uint16_t) {
  const uint8_t lo = pull_();
  const uint8_t hi = pull_();
  pc_ = ((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo)) + 1;
}

void CPU::rti_(uint16_t) {
  p_ = (pull_() & ~Break) | Unused;
  const uint8_t lo = pull_();
  const uint8_t hi = pull_();
  pc_ = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
}

void CPU::brk_(uint16_t) {
  const uint16_t ret = pc_ + 1;
  push_(ret >> 8);
  push_(ret & 0xFF);
  push_(p_ | Break | Unused);
  p_ |= Interrupt;
  pc_ = read16_(0xFFFE);
}

/* ======== STACK ======== */

void CPU::pha_(uint16_t) { push_(a_); }

void CPU::php_(uint16_t) { push_(p_ | Break | Unused); }

void CPU::pla_(uint16_t) {
  a_ = pull_();
  set_zn_(a_);
}

// Bits 4 and 5 do not exist in the register, so a pull cannot change them.
void CPU::plp_(uint16_t) { p_ = (pull_() & ~Break) | Unused; }

/* ======== FLAGS ======== */

void CPU::clc_(uint16_t) { set_flag_(Carry, false); }

void CPU::sec_(uint16_t) { set_flag_(Carry, true); }

void CPU::cli_(uint16_t) { set_flag_(Interrupt, false); }

void CPU::sei_(uint16_t) { set_flag_(Interrupt, true); }

void CPU::cld_(uint16_t) { set_flag_(Decimal, false); }

void CPU::sed_(uint16_t) { set_flag_(Decimal, true); }

void CPU::clv_(uint16_t) { set_flag_(Overflow, false); }

/* ======== OTHER ======== */

void CPU::nop_(uint16_t) {}

// The undocumented multi-byte NOPs still fetch their operand, which matters
// once reads have side effects.
void CPU::nop_read_(uint16_t addr) { bus_.read(addr); }

/* ======== UNOFFICIAL ======== */

void CPU::lax_(uint16_t addr) {
  a_ = bus_.read(addr);
  x_ = a_;
  set_zn_(a_);
}

void CPU::sax_(uint16_t addr) { bus_.write(addr, a_ & x_); }

void CPU::dcp_(uint16_t addr) {
  const uint8_t value = bus_.read(addr) - 1;
  bus_.write(addr, value);
  set_flag_(Carry, a_ >= value);
  set_zn_(static_cast<uint8_t>(a_ - value));
}

void CPU::isb_(uint16_t addr) {
  const uint8_t value = bus_.read(addr) + 1;
  bus_.write(addr, value);
  add_(~value);
}

void CPU::slo_(uint16_t addr) {
  const uint8_t value = shift_left_(bus_.read(addr));
  bus_.write(addr, value);
  a_ |= value;
  set_zn_(a_);
}

void CPU::rla_(uint16_t addr) {
  const uint8_t value = rotate_left_(bus_.read(addr));
  bus_.write(addr, value);
  a_ &= value;
  set_zn_(a_);
}

void CPU::sre_(uint16_t addr) {
  const uint8_t value = shift_right_(bus_.read(addr));
  bus_.write(addr, value);
  a_ ^= value;
  set_zn_(a_);
}

void CPU::rra_(uint16_t addr) {
  const uint8_t value = rotate_right_(bus_.read(addr));
  bus_.write(addr, value);
  add_(value);
}

void CPU::anc_(uint16_t addr) {
  a_ &= bus_.read(addr);
  set_zn_(a_);
  set_flag_(Carry, a_ & 0x80);
}

void CPU::alr_(uint16_t addr) {
  a_ = shift_right_(a_ & bus_.read(addr));
  set_zn_(a_);
}

// The rotate happens inside the adder, so carry and overflow come from the
// result's top two bits instead of the bit shifted out.
void CPU::arr_(uint16_t addr) {
  a_ = rotate_right_(a_ & bus_.read(addr));
  set_zn_(a_);
  set_flag_(Carry, a_ & 0x40);
  set_flag_(Overflow, ((a_ >> 6) ^ (a_ >> 5)) & 0x01);
}

void CPU::sbx_(uint16_t addr) {
  const uint8_t op = bus_.read(addr);
  const uint8_t lhs = a_ & x_;
  set_flag_(Carry, lhs >= op);
  x_ = lhs - op;
  set_zn_(x_);
}

void CPU::las_(uint16_t addr) {
  sp_ &= bus_.read(addr);
  a_ = sp_;
  x_ = sp_;
  set_zn_(a_);
}

// Unstable on real hardware: the result depends on analog behaviour of the
// accumulator bus. Treated here as the value every other emulator agrees on.
void CPU::ane_(uint16_t addr) {
  a_ &= x_ & bus_.read(addr);
  set_zn_(a_);
}

void CPU::sha_(uint16_t addr) { bus_.write(addr, a_ & x_ & ((addr >> 8) + 1)); }

void CPU::shx_(uint16_t addr) { bus_.write(addr, x_ & ((addr >> 8) + 1)); }

void CPU::shy_(uint16_t addr) { bus_.write(addr, y_ & ((addr >> 8) + 1)); }

void CPU::shs_(uint16_t addr) {
  sp_ = a_ & x_;
  bus_.write(addr, sp_ & ((addr >> 8) + 1));
}

void CPU::jam_(uint16_t) { halted_ = true; }

/* ======== DECODE TABLE ======== */

// OP is an instruction with fixed timing, OPX one that spends an extra cycle
// when indexing crosses a page boundary. Unofficial mnemonics carry a leading
// '*', the notation nestest.log uses.
#define OP(name, fn, mode, cycles)                                             \
  {name, &CPU::fn##_, Mode::mode, cycles, false}
#define OPX(name, fn, mode, cycles)                                            \
  {name, &CPU::fn##_, Mode::mode, cycles, true}

const CPU::Instruction CPU::table_[256] = {
    /* 00 */ OP("BRK", brk, Implied, 7),
    /* 01 */ OP("ORA", ora, IndirectX, 6),
    /* 02 */ OP("*JAM", jam, Implied, 2),
    /* 03 */ OP("*SLO", slo, IndirectX, 8),
    /* 04 */ OP("*NOP", nop_read, ZeroPage, 3),
    /* 05 */ OP("ORA", ora, ZeroPage, 3),
    /* 06 */ OP("ASL", asl, ZeroPage, 5),
    /* 07 */ OP("*SLO", slo, ZeroPage, 5),
    /* 08 */ OP("PHP", php, Implied, 3),
    /* 09 */ OP("ORA", ora, Immediate, 2),
    /* 0A */ OP("ASL", asl_a, Accumulator, 2),
    /* 0B */ OP("*ANC", anc, Immediate, 2),
    /* 0C */ OP("*NOP", nop_read, Absolute, 4),
    /* 0D */ OP("ORA", ora, Absolute, 4),
    /* 0E */ OP("ASL", asl, Absolute, 6),
    /* 0F */ OP("*SLO", slo, Absolute, 6),

    /* 10 */ OP("BPL", bpl, Relative, 2),
    /* 11 */ OPX("ORA", ora, IndirectY, 5),
    /* 12 */ OP("*JAM", jam, Implied, 2),
    /* 13 */ OP("*SLO", slo, IndirectY, 8),
    /* 14 */ OP("*NOP", nop_read, ZeroPageX, 4),
    /* 15 */ OP("ORA", ora, ZeroPageX, 4),
    /* 16 */ OP("ASL", asl, ZeroPageX, 6),
    /* 17 */ OP("*SLO", slo, ZeroPageX, 6),
    /* 18 */ OP("CLC", clc, Implied, 2),
    /* 19 */ OPX("ORA", ora, AbsoluteY, 4),
    /* 1A */ OP("*NOP", nop, Implied, 2),
    /* 1B */ OP("*SLO", slo, AbsoluteY, 7),
    /* 1C */ OPX("*NOP", nop_read, AbsoluteX, 4),
    /* 1D */ OPX("ORA", ora, AbsoluteX, 4),
    /* 1E */ OP("ASL", asl, AbsoluteX, 7),
    /* 1F */ OP("*SLO", slo, AbsoluteX, 7),

    /* 20 */ OP("JSR", jsr, Absolute, 6),
    /* 21 */ OP("AND", and, IndirectX, 6),
    /* 22 */ OP("*JAM", jam, Implied, 2),
    /* 23 */ OP("*RLA", rla, IndirectX, 8),
    /* 24 */ OP("BIT", bit, ZeroPage, 3),
    /* 25 */ OP("AND", and, ZeroPage, 3),
    /* 26 */ OP("ROL", rol, ZeroPage, 5),
    /* 27 */ OP("*RLA", rla, ZeroPage, 5),
    /* 28 */ OP("PLP", plp, Implied, 4),
    /* 29 */ OP("AND", and, Immediate, 2),
    /* 2A */ OP("ROL", rol_a, Accumulator, 2),
    /* 2B */ OP("*ANC", anc, Immediate, 2),
    /* 2C */ OP("BIT", bit, Absolute, 4),
    /* 2D */ OP("AND", and, Absolute, 4),
    /* 2E */ OP("ROL", rol, Absolute, 6),
    /* 2F */ OP("*RLA", rla, Absolute, 6),

    /* 30 */ OP("BMI", bmi, Relative, 2),
    /* 31 */ OPX("AND", and, IndirectY, 5),
    /* 32 */ OP("*JAM", jam, Implied, 2),
    /* 33 */ OP("*RLA", rla, IndirectY, 8),
    /* 34 */ OP("*NOP", nop_read, ZeroPageX, 4),
    /* 35 */ OP("AND", and, ZeroPageX, 4),
    /* 36 */ OP("ROL", rol, ZeroPageX, 6),
    /* 37 */ OP("*RLA", rla, ZeroPageX, 6),
    /* 38 */ OP("SEC", sec, Implied, 2),
    /* 39 */ OPX("AND", and, AbsoluteY, 4),
    /* 3A */ OP("*NOP", nop, Implied, 2),
    /* 3B */ OP("*RLA", rla, AbsoluteY, 7),
    /* 3C */ OPX("*NOP", nop_read, AbsoluteX, 4),
    /* 3D */ OPX("AND", and, AbsoluteX, 4),
    /* 3E */ OP("ROL", rol, AbsoluteX, 7),
    /* 3F */ OP("*RLA", rla, AbsoluteX, 7),

    /* 40 */ OP("RTI", rti, Implied, 6),
    /* 41 */ OP("EOR", eor, IndirectX, 6),
    /* 42 */ OP("*JAM", jam, Implied, 2),
    /* 43 */ OP("*SRE", sre, IndirectX, 8),
    /* 44 */ OP("*NOP", nop_read, ZeroPage, 3),
    /* 45 */ OP("EOR", eor, ZeroPage, 3),
    /* 46 */ OP("LSR", lsr, ZeroPage, 5),
    /* 47 */ OP("*SRE", sre, ZeroPage, 5),
    /* 48 */ OP("PHA", pha, Implied, 3),
    /* 49 */ OP("EOR", eor, Immediate, 2),
    /* 4A */ OP("LSR", lsr_a, Accumulator, 2),
    /* 4B */ OP("*ALR", alr, Immediate, 2),
    /* 4C */ OP("JMP", jmp, Absolute, 3),
    /* 4D */ OP("EOR", eor, Absolute, 4),
    /* 4E */ OP("LSR", lsr, Absolute, 6),
    /* 4F */ OP("*SRE", sre, Absolute, 6),

    /* 50 */ OP("BVC", bvc, Relative, 2),
    /* 51 */ OPX("EOR", eor, IndirectY, 5),
    /* 52 */ OP("*JAM", jam, Implied, 2),
    /* 53 */ OP("*SRE", sre, IndirectY, 8),
    /* 54 */ OP("*NOP", nop_read, ZeroPageX, 4),
    /* 55 */ OP("EOR", eor, ZeroPageX, 4),
    /* 56 */ OP("LSR", lsr, ZeroPageX, 6),
    /* 57 */ OP("*SRE", sre, ZeroPageX, 6),
    /* 58 */ OP("CLI", cli, Implied, 2),
    /* 59 */ OPX("EOR", eor, AbsoluteY, 4),
    /* 5A */ OP("*NOP", nop, Implied, 2),
    /* 5B */ OP("*SRE", sre, AbsoluteY, 7),
    /* 5C */ OPX("*NOP", nop_read, AbsoluteX, 4),
    /* 5D */ OPX("EOR", eor, AbsoluteX, 4),
    /* 5E */ OP("LSR", lsr, AbsoluteX, 7),
    /* 5F */ OP("*SRE", sre, AbsoluteX, 7),

    /* 60 */ OP("RTS", rts, Implied, 6),
    /* 61 */ OP("ADC", adc, IndirectX, 6),
    /* 62 */ OP("*JAM", jam, Implied, 2),
    /* 63 */ OP("*RRA", rra, IndirectX, 8),
    /* 64 */ OP("*NOP", nop_read, ZeroPage, 3),
    /* 65 */ OP("ADC", adc, ZeroPage, 3),
    /* 66 */ OP("ROR", ror, ZeroPage, 5),
    /* 67 */ OP("*RRA", rra, ZeroPage, 5),
    /* 68 */ OP("PLA", pla, Implied, 4),
    /* 69 */ OP("ADC", adc, Immediate, 2),
    /* 6A */ OP("ROR", ror_a, Accumulator, 2),
    /* 6B */ OP("*ARR", arr, Immediate, 2),
    /* 6C */ OP("JMP", jmp, Indirect, 5),
    /* 6D */ OP("ADC", adc, Absolute, 4),
    /* 6E */ OP("ROR", ror, Absolute, 6),
    /* 6F */ OP("*RRA", rra, Absolute, 6),

    /* 70 */ OP("BVS", bvs, Relative, 2),
    /* 71 */ OPX("ADC", adc, IndirectY, 5),
    /* 72 */ OP("*JAM", jam, Implied, 2),
    /* 73 */ OP("*RRA", rra, IndirectY, 8),
    /* 74 */ OP("*NOP", nop_read, ZeroPageX, 4),
    /* 75 */ OP("ADC", adc, ZeroPageX, 4),
    /* 76 */ OP("ROR", ror, ZeroPageX, 6),
    /* 77 */ OP("*RRA", rra, ZeroPageX, 6),
    /* 78 */ OP("SEI", sei, Implied, 2),
    /* 79 */ OPX("ADC", adc, AbsoluteY, 4),
    /* 7A */ OP("*NOP", nop, Implied, 2),
    /* 7B */ OP("*RRA", rra, AbsoluteY, 7),
    /* 7C */ OPX("*NOP", nop_read, AbsoluteX, 4),
    /* 7D */ OPX("ADC", adc, AbsoluteX, 4),
    /* 7E */ OP("ROR", ror, AbsoluteX, 7),
    /* 7F */ OP("*RRA", rra, AbsoluteX, 7),

    /* 80 */ OP("*NOP", nop_read, Immediate, 2),
    /* 81 */ OP("STA", sta, IndirectX, 6),
    /* 82 */ OP("*NOP", nop_read, Immediate, 2),
    /* 83 */ OP("*SAX", sax, IndirectX, 6),
    /* 84 */ OP("STY", sty, ZeroPage, 3),
    /* 85 */ OP("STA", sta, ZeroPage, 3),
    /* 86 */ OP("STX", stx, ZeroPage, 3),
    /* 87 */ OP("*SAX", sax, ZeroPage, 3),
    /* 88 */ OP("DEY", dey, Implied, 2),
    /* 89 */ OP("*NOP", nop_read, Immediate, 2),
    /* 8A */ OP("TXA", txa, Implied, 2),
    /* 8B */ OP("*ANE", ane, Immediate, 2),
    /* 8C */ OP("STY", sty, Absolute, 4),
    /* 8D */ OP("STA", sta, Absolute, 4),
    /* 8E */ OP("STX", stx, Absolute, 4),
    /* 8F */ OP("*SAX", sax, Absolute, 4),

    /* 90 */ OP("BCC", bcc, Relative, 2),
    /* 91 */ OP("STA", sta, IndirectY, 6),
    /* 92 */ OP("*JAM", jam, Implied, 2),
    /* 93 */ OP("*SHA", sha, IndirectY, 6),
    /* 94 */ OP("STY", sty, ZeroPageX, 4),
    /* 95 */ OP("STA", sta, ZeroPageX, 4),
    /* 96 */ OP("STX", stx, ZeroPageY, 4),
    /* 97 */ OP("*SAX", sax, ZeroPageY, 4),
    /* 98 */ OP("TYA", tya, Implied, 2),
    /* 99 */ OP("STA", sta, AbsoluteY, 5),
    /* 9A */ OP("TXS", txs, Implied, 2),
    /* 9B */ OP("*SHS", shs, AbsoluteY, 5),
    /* 9C */ OP("*SHY", shy, AbsoluteX, 5),
    /* 9D */ OP("STA", sta, AbsoluteX, 5),
    /* 9E */ OP("*SHX", shx, AbsoluteY, 5),
    /* 9F */ OP("*SHA", sha, AbsoluteY, 5),

    /* A0 */ OP("LDY", ldy, Immediate, 2),
    /* A1 */ OP("LDA", lda, IndirectX, 6),
    /* A2 */ OP("LDX", ldx, Immediate, 2),
    /* A3 */ OP("*LAX", lax, IndirectX, 6),
    /* A4 */ OP("LDY", ldy, ZeroPage, 3),
    /* A5 */ OP("LDA", lda, ZeroPage, 3),
    /* A6 */ OP("LDX", ldx, ZeroPage, 3),
    /* A7 */ OP("*LAX", lax, ZeroPage, 3),
    /* A8 */ OP("TAY", tay, Implied, 2),
    /* A9 */ OP("LDA", lda, Immediate, 2),
    /* AA */ OP("TAX", tax, Implied, 2),
    /* AB */ OP("*LXA", lax, Immediate, 2),
    /* AC */ OP("LDY", ldy, Absolute, 4),
    /* AD */ OP("LDA", lda, Absolute, 4),
    /* AE */ OP("LDX", ldx, Absolute, 4),
    /* AF */ OP("*LAX", lax, Absolute, 4),

    /* B0 */ OP("BCS", bcs, Relative, 2),
    /* B1 */ OPX("LDA", lda, IndirectY, 5),
    /* B2 */ OP("*JAM", jam, Implied, 2),
    /* B3 */ OPX("*LAX", lax, IndirectY, 5),
    /* B4 */ OP("LDY", ldy, ZeroPageX, 4),
    /* B5 */ OP("LDA", lda, ZeroPageX, 4),
    /* B6 */ OP("LDX", ldx, ZeroPageY, 4),
    /* B7 */ OP("*LAX", lax, ZeroPageY, 4),
    /* B8 */ OP("CLV", clv, Implied, 2),
    /* B9 */ OPX("LDA", lda, AbsoluteY, 4),
    /* BA */ OP("TSX", tsx, Implied, 2),
    /* BB */ OPX("*LAS", las, AbsoluteY, 4),
    /* BC */ OPX("LDY", ldy, AbsoluteX, 4),
    /* BD */ OPX("LDA", lda, AbsoluteX, 4),
    /* BE */ OPX("LDX", ldx, AbsoluteY, 4),
    /* BF */ OPX("*LAX", lax, AbsoluteY, 4),

    /* C0 */ OP("CPY", cpy, Immediate, 2),
    /* C1 */ OP("CMP", cmp, IndirectX, 6),
    /* C2 */ OP("*NOP", nop_read, Immediate, 2),
    /* C3 */ OP("*DCP", dcp, IndirectX, 8),
    /* C4 */ OP("CPY", cpy, ZeroPage, 3),
    /* C5 */ OP("CMP", cmp, ZeroPage, 3),
    /* C6 */ OP("DEC", dec, ZeroPage, 5),
    /* C7 */ OP("*DCP", dcp, ZeroPage, 5),
    /* C8 */ OP("INY", iny, Implied, 2),
    /* C9 */ OP("CMP", cmp, Immediate, 2),
    /* CA */ OP("DEX", dex, Implied, 2),
    /* CB */ OP("*SBX", sbx, Immediate, 2),
    /* CC */ OP("CPY", cpy, Absolute, 4),
    /* CD */ OP("CMP", cmp, Absolute, 4),
    /* CE */ OP("DEC", dec, Absolute, 6),
    /* CF */ OP("*DCP", dcp, Absolute, 6),

    /* D0 */ OP("BNE", bne, Relative, 2),
    /* D1 */ OPX("CMP", cmp, IndirectY, 5),
    /* D2 */ OP("*JAM", jam, Implied, 2),
    /* D3 */ OP("*DCP", dcp, IndirectY, 8),
    /* D4 */ OP("*NOP", nop_read, ZeroPageX, 4),
    /* D5 */ OP("CMP", cmp, ZeroPageX, 4),
    /* D6 */ OP("DEC", dec, ZeroPageX, 6),
    /* D7 */ OP("*DCP", dcp, ZeroPageX, 6),
    /* D8 */ OP("CLD", cld, Implied, 2),
    /* D9 */ OPX("CMP", cmp, AbsoluteY, 4),
    /* DA */ OP("*NOP", nop, Implied, 2),
    /* DB */ OP("*DCP", dcp, AbsoluteY, 7),
    /* DC */ OPX("*NOP", nop_read, AbsoluteX, 4),
    /* DD */ OPX("CMP", cmp, AbsoluteX, 4),
    /* DE */ OP("DEC", dec, AbsoluteX, 7),
    /* DF */ OP("*DCP", dcp, AbsoluteX, 7),

    /* E0 */ OP("CPX", cpx, Immediate, 2),
    /* E1 */ OP("SBC", sbc, IndirectX, 6),
    /* E2 */ OP("*NOP", nop_read, Immediate, 2),
    /* E3 */ OP("*ISB", isb, IndirectX, 8),
    /* E4 */ OP("CPX", cpx, ZeroPage, 3),
    /* E5 */ OP("SBC", sbc, ZeroPage, 3),
    /* E6 */ OP("INC", inc, ZeroPage, 5),
    /* E7 */ OP("*ISB", isb, ZeroPage, 5),
    /* E8 */ OP("INX", inx, Implied, 2),
    /* E9 */ OP("SBC", sbc, Immediate, 2),
    /* EA */ OP("NOP", nop, Implied, 2),
    /* EB */ OP("*SBC", sbc, Immediate, 2),
    /* EC */ OP("CPX", cpx, Absolute, 4),
    /* ED */ OP("SBC", sbc, Absolute, 4),
    /* EE */ OP("INC", inc, Absolute, 6),
    /* EF */ OP("*ISB", isb, Absolute, 6),

    /* F0 */ OP("BEQ", beq, Relative, 2),
    /* F1 */ OPX("SBC", sbc, IndirectY, 5),
    /* F2 */ OP("*JAM", jam, Implied, 2),
    /* F3 */ OP("*ISB", isb, IndirectY, 8),
    /* F4 */ OP("*NOP", nop_read, ZeroPageX, 4),
    /* F5 */ OP("SBC", sbc, ZeroPageX, 4),
    /* F6 */ OP("INC", inc, ZeroPageX, 6),
    /* F7 */ OP("*ISB", isb, ZeroPageX, 6),
    /* F8 */ OP("SED", sed, Implied, 2),
    /* F9 */ OPX("SBC", sbc, AbsoluteY, 4),
    /* FA */ OP("*NOP", nop, Implied, 2),
    /* FB */ OP("*ISB", isb, AbsoluteY, 7),
    /* FC */ OPX("*NOP", nop_read, AbsoluteX, 4),
    /* FD */ OPX("SBC", sbc, AbsoluteX, 4),
    /* FE */ OP("INC", inc, AbsoluteX, 7),
    /* FF */ OP("*ISB", isb, AbsoluteX, 7),
};

#undef OP
#undef OPX
