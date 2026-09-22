#ifndef BINDINGS_H
#define BINDINGS_H

#include <cstdint>

class Bindings {
public:
  static constexpr int kCount = 8;

  Bindings();

  uint8_t button_for(int key) const;
  int key(int index) const { return keys[index]; }
  const char *name(int index) const;
  void set_key(int index, int key);
  void reset();

private:
  void apply_defaults();
  void load();
  void save() const;

  int keys[kCount]{};
};

#endif
