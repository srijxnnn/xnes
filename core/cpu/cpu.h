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

  explicit CPU(CpuBus &b) : bus_(b) {}

  uint8_t a() const { return a_; }
  uint8_t x() const { return x_; }
  uint8_t y() const { return y_; }
  uint8_t p() const { return p_; }
  uint8_t sp() const { return sp_; }
  uint16_t pc() const { return pc_; }
  uint64_t cycles() const { return cycles_; }
  bool halted() const { return halted_; }

  void set_pc(uint16_t addr) { pc_ = addr; }

  void reset();

  // Latches an NMI to be taken before the next instruction. Ignores I.
  void nmi() { nmi_pending_ = true; }

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

  static const Instruction table_[256];

  CpuBus &bus_;

  uint8_t a_ = 0;
  uint8_t x_ = 0;
  uint8_t y_ = 0;
  uint8_t p_ = 0;
  uint8_t sp_ = 0;
  uint16_t pc_ = 0;
  uint64_t cycles_ = 0;
  bool page_crossed_ = false;
  bool halted_ = false;
  bool nmi_pending_ = false;

  static uint16_t make16_(uint8_t lo, uint8_t hi) {
    return static_cast<uint16_t>(static_cast<uint16_t>(hi) << 8 | lo);
  }

  uint16_t resolve_(Mode mode);
  uint16_t read16_(uint16_t addr);
  uint16_t read16_zp_(uint8_t addr);

  void push_(uint8_t value);
  uint8_t pull_();
  void push16_(uint16_t value);
  uint16_t pull16_();

  void set_flag_(uint8_t mask, bool value);
  void set_zn_(uint8_t value);
  void compare_(uint8_t reg, uint16_t addr);
  void branch_(bool condition, uint16_t target);
  void add_(uint8_t value);

  void load_(uint8_t &reg, uint16_t addr);
  void transfer_(uint8_t &dst, uint8_t src);

  // Pushes the return address and flags, blocks further IRQs, and jumps
  // through `vector`. Shared by BRK and the NMI sequence, which differ only in
  // the return address and whether the pushed B flag is set.
  void interrupt_(uint16_t vector, uint16_t ret, uint8_t flags);

  // Read-modify-write: the 6502 reads the operand, transforms it, and writes it
  // back to the same address. Returns the new value for the flags to use. The
  // transform is a template parameter rather than an argument so that it
  // inlines instead of becoming an indirect call on every such instruction.
  template <uint8_t (CPU::*Op)(uint8_t value)> uint8_t rmw_(uint16_t addr);

  uint8_t increment_(uint8_t value);
  uint8_t decrement_(uint8_t value);
  uint8_t shift_left_(uint8_t value);
  uint8_t shift_right_(uint8_t value);
  uint8_t rotate_left_(uint8_t value);
  uint8_t rotate_right_(uint8_t value);

  /* ======== ACCESS ======== */
  void lda_(uint16_t addr);
  void ldx_(uint16_t addr);
  void ldy_(uint16_t addr);
  void sta_(uint16_t addr);
  void stx_(uint16_t addr);
  void sty_(uint16_t addr);

  /* ======== TRANSFER ======== */
  void tax_(uint16_t addr);
  void tay_(uint16_t addr);
  void txa_(uint16_t addr);
  void tya_(uint16_t addr);
  void tsx_(uint16_t addr);
  void txs_(uint16_t addr);

  /* ======== ARITHMETIC ======== */
  void adc_(uint16_t addr);
  void sbc_(uint16_t addr);
  void inc_(uint16_t addr);
  void dec_(uint16_t addr);
  void inx_(uint16_t addr);
  void iny_(uint16_t addr);
  void dex_(uint16_t addr);
  void dey_(uint16_t addr);

  /* ======== SHIFT ======== */
  void asl_(uint16_t addr);
  void asl_a_(uint16_t addr);
  void lsr_(uint16_t addr);
  void lsr_a_(uint16_t addr);
  void rol_(uint16_t addr);
  void rol_a_(uint16_t addr);
  void ror_(uint16_t addr);
  void ror_a_(uint16_t addr);

  /* ======== BITWISE ======== */
  void and_(uint16_t addr);
  void ora_(uint16_t addr);
  void eor_(uint16_t addr);
  void bit_(uint16_t addr);

  /* ======== COMPARE ======== */
  void cmp_(uint16_t addr);
  void cpx_(uint16_t addr);
  void cpy_(uint16_t addr);

  /* ======== BRANCH ======== */
  void bcc_(uint16_t addr);
  void bcs_(uint16_t addr);
  void beq_(uint16_t addr);
  void bne_(uint16_t addr);
  void bpl_(uint16_t addr);
  void bmi_(uint16_t addr);
  void bvc_(uint16_t addr);
  void bvs_(uint16_t addr);

  /* ======== JUMP ======== */
  void jmp_(uint16_t addr);
  void jsr_(uint16_t addr);
  void rts_(uint16_t addr);
  void rti_(uint16_t addr);
  void brk_(uint16_t addr);
  void take_nmi_();

  /* ======== STACK ======== */
  void pha_(uint16_t addr);
  void php_(uint16_t addr);
  void pla_(uint16_t addr);
  void plp_(uint16_t addr);

  /* ======== FLAGS ======== */
  void clc_(uint16_t addr);
  void sec_(uint16_t addr);
  void cli_(uint16_t addr);
  void sei_(uint16_t addr);
  void cld_(uint16_t addr);
  void sed_(uint16_t addr);
  void clv_(uint16_t addr);

  /* ======== OTHER ======== */
  void nop_(uint16_t addr);
  void nop_read_(uint16_t addr);

  /* ======== UNOFFICIAL ======== */
  void lax_(uint16_t addr);
  void sax_(uint16_t addr);
  void dcp_(uint16_t addr);
  void isb_(uint16_t addr);
  void slo_(uint16_t addr);
  void rla_(uint16_t addr);
  void sre_(uint16_t addr);
  void rra_(uint16_t addr);
  void anc_(uint16_t addr);
  void alr_(uint16_t addr);
  void arr_(uint16_t addr);
  void sbx_(uint16_t addr);
  void las_(uint16_t addr);
  void ane_(uint16_t addr);
  void sha_(uint16_t addr);
  void shx_(uint16_t addr);
  void shy_(uint16_t addr);
  void shs_(uint16_t addr);
  void jam_(uint16_t addr);
};

#endif
