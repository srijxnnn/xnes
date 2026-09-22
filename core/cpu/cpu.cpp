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

void CPU::interrupt_(uint16_t vector, uint16_t ret, uint8_t flags) {
  push16_(ret);
  push_(flags);
  p_ |= Interrupt;
  pc_ = read16_(vector);
}

// BRK bills its 7 cycles through the decode table; an NMI has no table row, so
// it adds them here. The pushed B flag is what tells a handler the two apart.
void CPU::take_nmi_() {
  interrupt_(0xFFFA, pc_, (p_ & ~Break) | Unused);
  cycles_ += 7;
}

/* ======== ADDRESSING ======== */

// The two reads stay in named locals rather than being passed straight to
// make16_, because C++ does not order argument evaluation and a bus read can
// have side effects.
uint16_t CPU::read16_(uint16_t addr) {
  const uint8_t lo = bus_.read(addr);
  const uint8_t hi = bus_.read(addr + 1);
  return make16_(lo, hi);
}

// Pointers held in zero page wrap inside it: reading a pointer at $FF takes
// its high byte from $00, not $0100.
uint16_t CPU::read16_zp_(uint8_t addr) {
  const uint8_t lo = bus_.read(addr);
  const uint8_t hi = bus_.read(static_cast<uint8_t>(addr + 1));
  return make16_(lo, hi);
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
    return make16_(lo, hi);
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

void CPU::push16_(uint16_t value) {
  push_(value >> 8);
  push_(value & 0xFF);
}

uint16_t CPU::pull16_() {
  const uint8_t lo = pull_();
  const uint8_t hi = pull_();
  return make16_(lo, hi);
}

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

void CPU::load_(uint8_t &reg, uint16_t addr) {
  reg = bus_.read(addr);
  set_zn_(reg);
}

void CPU::transfer_(uint8_t &dst, uint8_t src) {
  dst = src;
  set_zn_(dst);
}

template <uint8_t (CPU::*Op)(uint8_t)> uint8_t CPU::rmw_(uint16_t addr) {
  const uint8_t value = (this->*Op)(bus_.read(addr));
  bus_.write(addr, value);
  return value;
}

uint8_t CPU::increment_(uint8_t value) { return value + 1; }

uint8_t CPU::decrement_(uint8_t value) { return value - 1; }

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

void CPU::lda_(uint16_t addr) { load_(a_, addr); }

void CPU::ldx_(uint16_t addr) { load_(x_, addr); }

void CPU::ldy_(uint16_t addr) { load_(y_, addr); }

void CPU::sta_(uint16_t addr) { bus_.write(addr, a_); }

void CPU::stx_(uint16_t addr) { bus_.write(addr, x_); }

void CPU::sty_(uint16_t addr) { bus_.write(addr, y_); }

/* ======== TRANSFER ======== */

void CPU::tax_(uint16_t) { transfer_(x_, a_); }

void CPU::tay_(uint16_t) { transfer_(y_, a_); }

void CPU::txa_(uint16_t) { transfer_(a_, x_); }

void CPU::tya_(uint16_t) { transfer_(a_, y_); }

void CPU::tsx_(uint16_t) { transfer_(x_, sp_); }

void CPU::txs_(uint16_t) { sp_ = x_; }

/* ======== ARITHMETIC ======== */

void CPU::adc_(uint16_t addr) { add_(bus_.read(addr)); }

void CPU::sbc_(uint16_t addr) { add_(~bus_.read(addr)); }

void CPU::inc_(uint16_t addr) { set_zn_(rmw_<&CPU::increment_>(addr)); }

void CPU::dec_(uint16_t addr) { set_zn_(rmw_<&CPU::decrement_>(addr)); }

void CPU::inx_(uint16_t) { set_zn_(++x_); }

void CPU::iny_(uint16_t) { set_zn_(++y_); }

void CPU::dex_(uint16_t) { set_zn_(--x_); }

void CPU::dey_(uint16_t) { set_zn_(--y_); }

/* ======== SHIFT ======== */

void CPU::asl_(uint16_t addr) { set_zn_(rmw_<&CPU::shift_left_>(addr)); }

void CPU::asl_a_(uint16_t) { transfer_(a_, shift_left_(a_)); }

void CPU::lsr_(uint16_t addr) { set_zn_(rmw_<&CPU::shift_right_>(addr)); }

void CPU::lsr_a_(uint16_t) { transfer_(a_, shift_right_(a_)); }

void CPU::rol_(uint16_t addr) { set_zn_(rmw_<&CPU::rotate_left_>(addr)); }

void CPU::rol_a_(uint16_t) { transfer_(a_, rotate_left_(a_)); }

void CPU::ror_(uint16_t addr) { set_zn_(rmw_<&CPU::rotate_right_>(addr)); }

void CPU::ror_a_(uint16_t) { transfer_(a_, rotate_right_(a_)); }

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
  push16_(pc_ - 1);
  pc_ = addr;
}

void CPU::rts_(uint16_t) { pc_ = pull16_() + 1; }

void CPU::rti_(uint16_t) {
  p_ = (pull_() & ~Break) | Unused;
  pc_ = pull16_();
}

void CPU::brk_(uint16_t) { interrupt_(0xFFFE, pc_ + 1, p_ | Break | Unused); }

/* ======== STACK ======== */

void CPU::pha_(uint16_t) { push_(a_); }

void CPU::php_(uint16_t) { push_(p_ | Break | Unused); }

void CPU::pla_(uint16_t) { transfer_(a_, pull_()); }

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
  load_(a_, addr);
  x_ = a_;
}

void CPU::sax_(uint16_t addr) { bus_.write(addr, a_ & x_); }

void CPU::dcp_(uint16_t addr) {
  const uint8_t value = rmw_<&CPU::decrement_>(addr);
  set_flag_(Carry, a_ >= value);
  set_zn_(static_cast<uint8_t>(a_ - value));
}

void CPU::isb_(uint16_t addr) { add_(~rmw_<&CPU::increment_>(addr)); }

void CPU::slo_(uint16_t addr) {
  transfer_(a_, a_ | rmw_<&CPU::shift_left_>(addr));
}

void CPU::rla_(uint16_t addr) {
  transfer_(a_, a_ & rmw_<&CPU::rotate_left_>(addr));
}

void CPU::sre_(uint16_t addr) {
  transfer_(a_, a_ ^ rmw_<&CPU::shift_right_>(addr));
}

void CPU::rra_(uint16_t addr) { add_(rmw_<&CPU::rotate_right_>(addr)); }

void CPU::anc_(uint16_t addr) {
  a_ &= bus_.read(addr);
  set_zn_(a_);
  set_flag_(Carry, a_ & 0x80);
}

void CPU::alr_(uint16_t addr) {
  transfer_(a_, shift_right_(a_ & bus_.read(addr)));
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
  transfer_(a_, sp_);
  x_ = sp_;
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
