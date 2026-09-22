#include "controller.h"

void Controller::write(uint8_t data) {
  const bool strobe = data & 1;
  if (strobe_ && !strobe) {
    snapshot_ = buttons_;
    index_ = 0;
  }
  strobe_ = strobe;
}

uint8_t Controller::read() {
  if (strobe_) {
    return buttons_ & 1;
  }
  if (index_ >= 8) {
    return 1;
  }
  return (snapshot_ >> index_++) & 1;
}
