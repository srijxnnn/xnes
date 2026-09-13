#ifndef LOGGER_H
#define LOGGER_H

#include "cpu.h"

class Logger {
public:
  void log(const CPU &cpu) const;
};

#endif
