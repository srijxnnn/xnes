#include "cpu.h"
#include <cstdint>

void CPU::reset() {
  a = 0;
  x = 0;
  y = 0;
  p = Interrupt | Unused;
  sp = 0xFD;
  pc = read16(0xFFFC);
  cycles = 7;
  halted = false;
  nmi_pending = false;
}

int CPU::step() {
  const uint64_t start = cycles;

  if (halted) {
    cycles += 2;
    return 2;
  }

  if (nmi_pending) {
    nmi_pending = false;
    take_nmi();
  } else {
    const uint8_t opcode = bus.read(pc++);
    const Instruction &in = table[opcode];

    page_crossed = false;
    const uint16_t addr = resolve(in.mode);

    cycles += in.cycles;
    if (in.page_penalty && page_crossed) {
      cycles++;
    }

    // Taken branches bill their own extra cycles from inside the handler.
    (this->*in.exec)(addr);
  }

  cycles += bus.drain_stall();
  return static_cast<int>(cycles - start);
}

void CPU::interrupt(uint16_t vector, uint16_t ret, uint8_t flags) {
  push16(ret);
  push(flags);
  p |= Interrupt;
  pc = read16(vector);
}

// BRK bills its 7 cycles through the decode table; an NMI has no table row, so
// it adds them here. The pushed B flag is what tells a handler the two apart.
void CPU::take_nmi() {
  interrupt(0xFFFA, pc, (p & ~Break) | Unused);
  cycles += 7;
}

/* ======== ADDRESSING ======== */

uint16_t CPU::read16(uint16_t addr) {
  const uint8_t lo = bus.read(addr);
  const uint8_t hi = bus.read(addr + 1);
  return make16(lo, hi);
}
uint16_t CPU::read16_zp(uint8_t addr) {
  const uint8_t lo = bus.read(addr);
  const uint8_t hi = bus.read(static_cast<uint8_t>(addr + 1));
  return make16(lo, hi);
}

uint16_t CPU::resolve(Mode mode) {
  switch (mode) {
  case Mode::Implied:
  case Mode::Accumulator:
    return 0;
  case Mode::Immediate:
    return pc++;
  case Mode::ZeroPage:
    return bus.read(pc++);
  case Mode::ZeroPageX:
    return static_cast<uint8_t>(bus.read(pc++) + x);
  case Mode::ZeroPageY:
    return static_cast<uint8_t>(bus.read(pc++) + y);
  case Mode::Absolute: {
    const uint16_t addr = read16(pc);
    pc += 2;
    return addr;
  }
  case Mode::AbsoluteX: {
    const uint16_t base = read16(pc);
    pc += 2;
    const uint16_t addr = base + x;
    page_crossed = (base & 0xFF00) != (addr & 0xFF00);
    return addr;
  }
  case Mode::AbsoluteY: {
    const uint16_t base = read16(pc);
    pc += 2;
    const uint16_t addr = base + y;
    page_crossed = (base & 0xFF00) != (addr & 0xFF00);
    return addr;
  }
  case Mode::Indirect: {
    const uint16_t ptr = read16(pc);
    pc += 2;
    const uint8_t lo = bus.read(ptr);
    const uint8_t hi = bus.read((ptr & 0xFF00) | static_cast<uint8_t>(ptr + 1));
    return make16(lo, hi);
  }
  case Mode::IndirectX:
    return read16_zp(static_cast<uint8_t>(bus.read(pc++) + x));
  case Mode::IndirectY: {
    const uint16_t base = read16_zp(bus.read(pc++));
    const uint16_t addr = base + y;
    page_crossed = (base & 0xFF00) != (addr & 0xFF00);
    return addr;
  }
  case Mode::Relative: {
    const int8_t offset = static_cast<int8_t>(bus.read(pc++));
    return pc + offset;
  }
  }

  return 0;
}

/* ======== PRIMITIVES ======== */

void CPU::push(uint8_t value) { bus.write(0x0100 | sp--, value); }
uint8_t CPU::pull() { return bus.read(0x0100 | ++sp); }
void CPU::push16(uint16_t value) {
  push(value >> 8);
  push(value & 0xFF);
}
uint16_t CPU::pull16() {
  const uint8_t lo = pull();
  const uint8_t hi = pull();
  return make16(lo, hi);
}
void CPU::set_flag(uint8_t mask, bool value) {
  if (value) {
    p |= mask;
  } else {
    p &= ~mask;
  }
}
void CPU::set_zn(uint8_t value) {
  set_flag(Zero, value == 0);
  set_flag(Negative, value & 0x80);
}
void CPU::compare(uint8_t reg, uint16_t addr) {
  const uint8_t op = bus.read(addr);
  set_flag(Carry, reg >= op);
  set_zn(static_cast<uint8_t>(reg - op));
}
void CPU::branch(bool condition, uint16_t target) {
  if (!condition) {
    return;
  }

  cycles += (pc & 0xFF00) == (target & 0xFF00) ? 1 : 2;
  pc = target;
}
void CPU::add(uint8_t value) {
  const uint16_t result = a + value + (p & Carry);
  set_flag(Carry, result > 0xFF);
  set_flag(Overflow, (a ^ result) & (value ^ result) & 0x80);
  a = static_cast<uint8_t>(result);
  set_zn(a);
}
void CPU::load(uint8_t &reg, uint16_t addr) {
  reg = bus.read(addr);
  set_zn(reg);
}
void CPU::transfer(uint8_t &dst, uint8_t src) {
  dst = src;
  set_zn(dst);
}
template <uint8_t (CPU::*Op)(uint8_t)> uint8_t CPU::rmw(uint16_t addr) {
  const uint8_t value = (this->*Op)(bus.read(addr));
  bus.write(addr, value);
  return value;
}

uint8_t CPU::increment(uint8_t value) { return value + 1; }
uint8_t CPU::decrement(uint8_t value) { return value - 1; }
uint8_t CPU::shift_left(uint8_t value) {
  set_flag(Carry, value & 0x80);
  return value << 1;
}
uint8_t CPU::shift_right(uint8_t value) {
  set_flag(Carry, value & 0x01);
  return value >> 1;
}
uint8_t CPU::rotate_left(uint8_t value) {
  const uint8_t carry = p & Carry;
  set_flag(Carry, value & 0x80);
  return (value << 1) | carry;
}
uint8_t CPU::rotate_right(uint8_t value) {
  const uint8_t carry = p & Carry;
  set_flag(Carry, value & 0x01);
  return (value >> 1) | (carry << 7);
}

/* ======== ACCESS ======== */

void CPU::lda(uint16_t addr) { load(a, addr); }
void CPU::ldx(uint16_t addr) { load(x, addr); }
void CPU::ldy(uint16_t addr) { load(y, addr); }
void CPU::sta(uint16_t addr) { bus.write(addr, a); }
void CPU::stx(uint16_t addr) { bus.write(addr, x); }
void CPU::sty(uint16_t addr) { bus.write(addr, y); }

/* ======== TRANSFER ======== */

void CPU::tax(uint16_t) { transfer(x, a); }
void CPU::tay(uint16_t) { transfer(y, a); }
void CPU::txa(uint16_t) { transfer(a, x); }
void CPU::tya(uint16_t) { transfer(a, y); }
void CPU::tsx(uint16_t) { transfer(x, sp); }
void CPU::txs(uint16_t) { sp = x; }

/* ======== ARITHMETIC ======== */

void CPU::adc(uint16_t addr) { add(bus.read(addr)); }
void CPU::sbc(uint16_t addr) { add(~bus.read(addr)); }
void CPU::inc(uint16_t addr) { set_zn(rmw<&CPU::increment>(addr)); }
void CPU::dec(uint16_t addr) { set_zn(rmw<&CPU::decrement>(addr)); }
void CPU::inx(uint16_t) { set_zn(++x); }
void CPU::iny(uint16_t) { set_zn(++y); }
void CPU::dex(uint16_t) { set_zn(--x); }
void CPU::dey(uint16_t) { set_zn(--y); }

/* ======== SHIFT ======== */

void CPU::asl(uint16_t addr) { set_zn(rmw<&CPU::shift_left>(addr)); }
void CPU::asl_a(uint16_t) { transfer(a, shift_left(a)); }
void CPU::lsr(uint16_t addr) { set_zn(rmw<&CPU::shift_right>(addr)); }
void CPU::lsr_a(uint16_t) { transfer(a, shift_right(a)); }
void CPU::rol(uint16_t addr) { set_zn(rmw<&CPU::rotate_left>(addr)); }
void CPU::rol_a(uint16_t) { transfer(a, rotate_left(a)); }
void CPU::ror(uint16_t addr) { set_zn(rmw<&CPU::rotate_right>(addr)); }
void CPU::ror_a(uint16_t) { transfer(a, rotate_right(a)); }

/* ======== BITWISE ======== */

void CPU::and_op(uint16_t addr) {
  a &= bus.read(addr);
  set_zn(a);
}
void CPU::ora(uint16_t addr) {
  a |= bus.read(addr);
  set_zn(a);
}
void CPU::eor(uint16_t addr) {
  a ^= bus.read(addr);
  set_zn(a);
}
void CPU::bit(uint16_t addr) {
  const uint8_t mem = bus.read(addr);
  set_flag(Zero, (a & mem) == 0);
  set_flag(Negative, mem & 0x80);
  set_flag(Overflow, mem & 0x40);
}

/* ======== COMPARE ======== */

void CPU::cmp(uint16_t addr) { compare(a, addr); }
void CPU::cpx(uint16_t addr) { compare(x, addr); }
void CPU::cpy(uint16_t addr) { compare(y, addr); }

/* ======== BRANCH ======== */

void CPU::bcc(uint16_t addr) { branch((p & Carry) == 0, addr); }
void CPU::bcs(uint16_t addr) { branch(p & Carry, addr); }
void CPU::beq(uint16_t addr) { branch(p & Zero, addr); }
void CPU::bne(uint16_t addr) { branch((p & Zero) == 0, addr); }
void CPU::bpl(uint16_t addr) { branch((p & Negative) == 0, addr); }
void CPU::bmi(uint16_t addr) { branch(p & Negative, addr); }
void CPU::bvc(uint16_t addr) { branch((p & Overflow) == 0, addr); }
void CPU::bvs(uint16_t addr) { branch(p & Overflow, addr); }

/* ======== JUMP ======== */

void CPU::jmp(uint16_t addr) { pc = addr; }
void CPU::jsr(uint16_t addr) {
  push16(pc - 1);
  pc = addr;
}
void CPU::rts(uint16_t) { pc = pull16() + 1; }
void CPU::rti(uint16_t) {
  p = (pull() & ~Break) | Unused;
  pc = pull16();
}
void CPU::brk(uint16_t) { interrupt(0xFFFE, pc + 1, p | Break | Unused); }

/* ======== STACK ======== */

void CPU::pha(uint16_t) { push(a); }
void CPU::php(uint16_t) { push(p | Break | Unused); }
void CPU::pla(uint16_t) { transfer(a, pull()); }

// Bits 4 and 5 do not exist in the register, so a pull cannot change them.
void CPU::plp(uint16_t) { p = (pull() & ~Break) | Unused; }

/* ======== FLAGS ======== */

void CPU::clc(uint16_t) { set_flag(Carry, false); }
void CPU::sec(uint16_t) { set_flag(Carry, true); }
void CPU::cli(uint16_t) { set_flag(Interrupt, false); }
void CPU::sei(uint16_t) { set_flag(Interrupt, true); }
void CPU::cld(uint16_t) { set_flag(Decimal, false); }
void CPU::sed(uint16_t) { set_flag(Decimal, true); }
void CPU::clv(uint16_t) { set_flag(Overflow, false); }

/* ======== OTHER ======== */

void CPU::nop(uint16_t) {}
void CPU::nop_read(uint16_t addr) { bus.read(addr); }

/* ======== UNOFFICIAL ======== */

void CPU::lax(uint16_t addr) {
  load(a, addr);
  x = a;
}
void CPU::sax(uint16_t addr) { bus.write(addr, a & x); }
void CPU::dcp(uint16_t addr) {
  const uint8_t value = rmw<&CPU::decrement>(addr);
  set_flag(Carry, a >= value);
  set_zn(static_cast<uint8_t>(a - value));
}
void CPU::isb(uint16_t addr) { add(~rmw<&CPU::increment>(addr)); }
void CPU::slo(uint16_t addr) { transfer(a, a | rmw<&CPU::shift_left>(addr)); }
void CPU::rla(uint16_t addr) { transfer(a, a & rmw<&CPU::rotate_left>(addr)); }
void CPU::sre(uint16_t addr) { transfer(a, a ^ rmw<&CPU::shift_right>(addr)); }
void CPU::rra(uint16_t addr) { add(rmw<&CPU::rotate_right>(addr)); }
void CPU::anc(uint16_t addr) {
  a &= bus.read(addr);
  set_zn(a);
  set_flag(Carry, a & 0x80);
}
void CPU::alr(uint16_t addr) { transfer(a, shift_right(a & bus.read(addr))); }
void CPU::arr(uint16_t addr) {
  a = rotate_right(a & bus.read(addr));
  set_zn(a);
  set_flag(Carry, a & 0x40);
  set_flag(Overflow, ((a >> 6) ^ (a >> 5)) & 0x01);
}
void CPU::sbx(uint16_t addr) {
  const uint8_t op = bus.read(addr);
  const uint8_t lhs = a & x;
  set_flag(Carry, lhs >= op);
  x = lhs - op;
  set_zn(x);
}
void CPU::las(uint16_t addr) {
  sp &= bus.read(addr);
  transfer(a, sp);
  x = sp;
}
void CPU::ane(uint16_t addr) {
  a &= x & bus.read(addr);
  set_zn(a);
}
void CPU::sha(uint16_t addr) { bus.write(addr, a & x & ((addr >> 8) + 1)); }
void CPU::shx(uint16_t addr) { bus.write(addr, x & ((addr >> 8) + 1)); }
void CPU::shy(uint16_t addr) { bus.write(addr, y & ((addr >> 8) + 1)); }
void CPU::shs(uint16_t addr) {
  sp = a & x;
  bus.write(addr, sp & ((addr >> 8) + 1));
}
void CPU::jam(uint16_t) { halted = true; }
