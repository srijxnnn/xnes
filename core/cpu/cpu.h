#ifndef CPU_H
#define CPU_H

#include "bus/cpu_bus.h"
#include <cstdint>

class CPU {
public:
  enum Flag : uint8_t {
    Carry = 0x01,
    Zero = 0x02,
    Interrupt = 0x04,
    Decimal = 0x08,
    Break = 0x10,
    Unused = 0x20,
    Overflow = 0x40,
    Negative = 0x80,
  };

  explicit CPU(CpuBus &b) : bus(b) {}

  uint8_t a = 0;
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t p = 0;
  uint8_t sp = 0;
  uint16_t pc = 0;
  uint64_t cycles = 0;
  bool halted = false;

  void set_pc(uint16_t addr) { pc = addr; }

  void reset();

  // Latches an NMI to be taken before the next instruction. Ignores I.
  void nmi() { nmi_pending = true; }

  // Runs one instruction (or a pending NMI) and reports how many cycles it
  // took, so the caller can advance the PPU by the same amount of time.
  int step();

private:
  enum class Mode : uint8_t {
    Implied,
    Accumulator,
    Immediate,
    ZeroPage,
    ZeroPageX,
    ZeroPageY,
    Absolute,
    AbsoluteX,
    AbsoluteY,
    Indirect,
    IndirectX,
    IndirectY,
    Relative,
  };

  // One row per opcode. `exec` always receives an effective address; implied
  // and accumulator handlers ignore it. `page_penalty` marks the read-only
  // indexed modes that cost an extra cycle when indexing crosses a page.
  // `name` is the nestest mnemonic, which handlers alone do not give: it keeps
  // the '*' on unofficial opcodes and the real name where a handler is shared.
  struct Instruction {
    const char *name;
    void (CPU::*exec)(uint16_t addr);
    Mode mode;
    uint8_t cycles;
    bool page_penalty;
  };

  static const Instruction table[256];

  CpuBus &bus;

  bool page_crossed = false;
  bool nmi_pending = false;

  static uint16_t make16(uint8_t lo, uint8_t hi) {
    return static_cast<uint16_t>(static_cast<uint16_t>(hi) << 8 | lo);
  }

  uint16_t resolve(Mode mode);
  uint16_t read16(uint16_t addr);
  uint16_t read16_zp(uint8_t addr);

  void push(uint8_t value);
  uint8_t pull();
  void push16(uint16_t value);
  uint16_t pull16();

  void set_flag(uint8_t mask, bool value);
  void set_zn(uint8_t value);
  void compare(uint8_t reg, uint16_t addr);
  void branch(bool condition, uint16_t target);
  void add(uint8_t value);

  void load(uint8_t &reg, uint16_t addr);
  void transfer(uint8_t &dst, uint8_t src);

  void interrupt(uint16_t vector, uint16_t ret, uint8_t flags);
  template <uint8_t (CPU::*Op)(uint8_t value)> uint8_t rmw(uint16_t addr);

  uint8_t increment(uint8_t value);
  uint8_t decrement(uint8_t value);
  uint8_t shift_left(uint8_t value);
  uint8_t shift_right(uint8_t value);
  uint8_t rotate_left(uint8_t value);
  uint8_t rotate_right(uint8_t value);

  /* ======== ACCESS ======== */
  void lda(uint16_t addr);
  void ldx(uint16_t addr);
  void ldy(uint16_t addr);
  void sta(uint16_t addr);
  void stx(uint16_t addr);
  void sty(uint16_t addr);

  /* ======== TRANSFER ======== */
  void tax(uint16_t addr);
  void tay(uint16_t addr);
  void txa(uint16_t addr);
  void tya(uint16_t addr);
  void tsx(uint16_t addr);
  void txs(uint16_t addr);

  /* ======== ARITHMETIC ======== */
  void adc(uint16_t addr);
  void sbc(uint16_t addr);
  void inc(uint16_t addr);
  void dec(uint16_t addr);
  void inx(uint16_t addr);
  void iny(uint16_t addr);
  void dex(uint16_t addr);
  void dey(uint16_t addr);

  /* ======== SHIFT ======== */
  void asl(uint16_t addr);
  void asl_a(uint16_t addr);
  void lsr(uint16_t addr);
  void lsr_a(uint16_t addr);
  void rol(uint16_t addr);
  void rol_a(uint16_t addr);
  void ror(uint16_t addr);
  void ror_a(uint16_t addr);

  /* ======== BITWISE ======== */
  void and_op(uint16_t addr);
  void ora(uint16_t addr);
  void eor(uint16_t addr);
  void bit(uint16_t addr);

  /* ======== COMPARE ======== */
  void cmp(uint16_t addr);
  void cpx(uint16_t addr);
  void cpy(uint16_t addr);

  /* ======== BRANCH ======== */
  void bcc(uint16_t addr);
  void bcs(uint16_t addr);
  void beq(uint16_t addr);
  void bne(uint16_t addr);
  void bpl(uint16_t addr);
  void bmi(uint16_t addr);
  void bvc(uint16_t addr);
  void bvs(uint16_t addr);

  /* ======== JUMP ======== */
  void jmp(uint16_t addr);
  void jsr(uint16_t addr);
  void rts(uint16_t addr);
  void rti(uint16_t addr);
  void brk(uint16_t addr);
  void take_nmi();

  /* ======== STACK ======== */
  void pha(uint16_t addr);
  void php(uint16_t addr);
  void pla(uint16_t addr);
  void plp(uint16_t addr);

  /* ======== FLAGS ======== */
  void clc(uint16_t addr);
  void sec(uint16_t addr);
  void cli(uint16_t addr);
  void sei(uint16_t addr);
  void cld(uint16_t addr);
  void sed(uint16_t addr);
  void clv(uint16_t addr);

  /* ======== OTHER ======== */
  void nop(uint16_t addr);
  void nop_read(uint16_t addr);

  /* ======== UNOFFICIAL ======== */
  void lax(uint16_t addr);
  void sax(uint16_t addr);
  void dcp(uint16_t addr);
  void isb(uint16_t addr);
  void slo(uint16_t addr);
  void rla(uint16_t addr);
  void sre(uint16_t addr);
  void rra(uint16_t addr);
  void anc(uint16_t addr);
  void alr(uint16_t addr);
  void arr(uint16_t addr);
  void sbx(uint16_t addr);
  void las(uint16_t addr);
  void ane(uint16_t addr);
  void sha(uint16_t addr);
  void shx(uint16_t addr);
  void shy(uint16_t addr);
  void shs(uint16_t addr);
  void jam(uint16_t addr);
};

#endif
