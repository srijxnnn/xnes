#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

enum class Mirror : uint8_t { Horizontal, Vertical, Four };

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
