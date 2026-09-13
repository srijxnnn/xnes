#include "cartridge.h"
#include <array>
#include <fstream>

std::optional<Cartridge> Cartridge::load(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return std::nullopt;
  }

  std::array<uint8_t, 16> header{};
  file.read(reinterpret_cast<char *>(header.data()), header.size());
  if (file.gcount() != 16) {
    return std::nullopt;
  }

  if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' ||
      header[3] != 0x1A) {
    return std::nullopt;
  }

  const std::size_t prg_size = static_cast<std::size_t>(header[4]) * 16384;
  const std::size_t chr_size = static_cast<std::size_t>(header[5]) * 8192;

  Cartridge cart;

  cart.prg_.resize(prg_size);

  file.read(reinterpret_cast<char *>(cart.prg_.data()), prg_size);
  if (file.gcount() != static_cast<std::streamsize>(prg_size)) {
    return std::nullopt;
  }

  if (chr_size == 0) {
    cart.chr_.assign(8192, 0);

  } else {
    cart.chr_.resize(chr_size);
    file.read(reinterpret_cast<char *>(cart.chr_.data()), chr_size);
    if (file.gcount() != static_cast<std::streamsize>(chr_size)) {
      return std::nullopt;
    }
  }

  return cart;
}