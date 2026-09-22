#ifndef TRACE_H
#define TRACE_H

#include "cpu/cpu.h"

#include <string>

// The debug window is refreshed from this tail once per frame. Inserting
// every instruction into a text widget cannot finish in 16ms.
class Trace {
public:
  void clear();
  void record(const CPU &cpu);
  std::string text() const;

private:
  struct Line {
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t p;
    uint8_t sp;
    uint64_t cycles;
  };

  static constexpr int kLines = 24;

  Line lines[kLines]{};
  int at = 0;
  int count = 0;
};

#endif
