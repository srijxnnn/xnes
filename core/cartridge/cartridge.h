#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

enum class Mirror : uint8_t { Horizontal, Vertical, Four };

// iNES cartridge. MVP only maps NROM (mapper 0), which is what Donkey Kong
// and nestest both use.
class Cartridge {
  std::vector<uint8_t> prg_;
  std::vector<uint8_t> chr_;
  std::array<uint8_t, 8192> prg_ram_{};
  Mirror mirror_ = Mirror::Vertical;
  bool chr_ram_ = false;

public:
  static std::optional<Cartridge> load(const std::filesystem::path &path);

  uint8_t cpu_read(uint16_t addr) const;
  void cpu_write(uint16_t addr, uint8_t data);

  uint8_t chr_read(uint16_t addr) const;
  void chr_write(uint16_t addr, uint8_t data);

  Mirror mirror() const { return mirror_; }
};

#endif
