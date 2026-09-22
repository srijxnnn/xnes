#include "trace.h"

#include <cstdio>

void Trace::clear() {
  at = 0;
  count = 0;
}

void Trace::record(const CPU &cpu) {
  Line &line = lines[at];
  line.pc = cpu.pc;
  line.a = cpu.a;
  line.x = cpu.x;
  line.y = cpu.y;
  line.p = cpu.p;
  line.sp = cpu.sp;
  line.cycles = cpu.cycles;
  at = (at + 1) % kLines;
  if (count < kLines) {
    count++;
  }
}

std::string Trace::text() const {
  const int start = count < kLines ? 0 : at;
  std::string text;
  text.reserve(static_cast<size_t>(count) * 48);
  for (int i = 0; i < count; i++) {
    const Line &line = lines[(start + i) % kLines];
    char row[64];
    std::snprintf(row, sizeof row,
                  "%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%llu\n",
                  line.pc, line.a, line.x, line.y, line.p, line.sp,
                  static_cast<unsigned long long>(line.cycles));
    text.append(row);
  }
  return text;
}
