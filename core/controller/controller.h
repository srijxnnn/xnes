#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <cstdint>

class Controller {
public:
  enum Button : uint8_t {
    A = 1 << 0,
    B = 1 << 1,
    Select = 1 << 2,
    Start = 1 << 3,
    Up = 1 << 4,
    Down = 1 << 5,
    Left = 1 << 6,
    Right = 1 << 7,
  };

  void set(uint8_t value) { buttons = value; }

  void write(uint8_t data);
  uint8_t read();

private:
  uint8_t buttons = 0;
  uint8_t snapshot = 0;
  uint8_t index = 0;
  bool strobe = false;
};

#endif
