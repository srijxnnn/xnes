#include "logger.h"
#include <cstdio>

void Logger::log(const CPU &cpu) const {
  std::printf("%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", cpu.pc(), cpu.a(),
              cpu.x(), cpu.y(), cpu.p(), cpu.sp());
}