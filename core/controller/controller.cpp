#include "controller.h"

void Controller::write(uint8_t data) {
  const bool next = data & 1;
  if (strobe && !next) {
    snapshot = buttons;
    index = 0;
  }
  strobe = next;
}

uint8_t Controller::read() {
  if (strobe) {
    return buttons & 1;
  }
  if (index >= 8) {
    return 1;
  }
  return (snapshot >> index++) & 1;
}
