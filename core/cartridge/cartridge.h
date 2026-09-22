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

  int mapper_id() const { return mapper_id_; }
  Mirror mirror() const { return mirror_; }
  bool chr_is_ram() const { return chr_ram_; }

  std::size_t prg_size() const { return prg_.size(); }
  std::size_t chr_size() const { return chr_.size(); }

  uint8_t prg(std::size_t offset) const { return prg_[offset]; }
  uint8_t chr(std::size_t offset) const { return chr_[offset]; }
  void set_chr(std::size_t offset, uint8_t data) { chr_[offset] = data; }

  uint8_t prg_ram(std::size_t offset) const { return prg_ram_[offset]; }
  void set_prg_ram(std::size_t offset, uint8_t data) {
    prg_ram_[offset] = data;
  }

private:
  std::vector<uint8_t> prg_;
  std::vector<uint8_t> chr_;
  std::array<uint8_t, 8192> prg_ram_{};
  Mirror mirror_ = Mirror::Vertical;
  bool chr_ram_ = false;
  int mapper_id_ = 0;
};

#endif
