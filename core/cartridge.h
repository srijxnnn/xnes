#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

class Cartridge {
  std::vector<uint8_t> prg_;
  std::vector<uint8_t> chr_;

public:
  static std::optional<Cartridge> load(const std::filesystem::path &path);

  const std::vector<uint8_t> &prg() const { return prg_; }
  const std::vector<uint8_t> &chr() const { return chr_; }
};

#endif