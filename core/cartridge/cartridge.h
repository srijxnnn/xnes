#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

enum class Mirror : uint8_t { Horizontal, Vertical, Four };

// A parsed iNES image plus the board's work RAM: everything the file says, and
// nothing about how a board maps it. Which addresses reach which bytes is the
// mapper's job, so this class does not know about $8000 or $6000.
class Cartridge {
public:
  static std::optional<Cartridge> load(const std::filesystem::path &path);

  std::vector<uint8_t> prg;
  std::vector<uint8_t> chr;
  std::array<uint8_t, 8192> prg_ram{};
  Mirror mirror = Mirror::Vertical;
  bool chr_ram = false;
  int mapper_id = 0;
};

#endif
