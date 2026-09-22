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
  if (prg_size == 0) {
    return std::nullopt;
  }

  Cartridge cart;

  // The mapper number is split across the header, and iNES 2.0 adds a third
  // nibble. Whether the board is supported is decided by create_mapper, not
  // here: this function only reports what the file claims.
  cart.mapper_id = header[6] >> 4;
  if ((header[7] & 0x0C) == 0x08) {
    cart.mapper_id |=
        (header[7] & 0xF0) | (static_cast<int>(header[8] & 0x0F) << 8);
  } else if ((header[7] & 0x0C) == 0x00) {
    cart.mapper_id |= header[7] & 0xF0;
  }

  if (header[6] & 0x08) {
    cart.mirror = Mirror::Four;
  } else if (header[6] & 0x01) {
    cart.mirror = Mirror::Vertical;
  } else {
    cart.mirror = Mirror::Horizontal;
  }

  if (header[6] & 0x04) {
    file.seekg(512, std::ios::cur);
    if (!file) {
      return std::nullopt;
    }
  }

  cart.prg.resize(prg_size);
  file.read(reinterpret_cast<char *>(cart.prg.data()),
            static_cast<std::streamsize>(prg_size));
  if (file.gcount() != static_cast<std::streamsize>(prg_size)) {
    return std::nullopt;
  }

  // A zero CHR size means the board carries CHR RAM instead of ROM.
  if (chr_size == 0) {
    cart.chr.assign(8192, 0);
    cart.chr_ram = true;
  } else {
    cart.chr.resize(chr_size);
    file.read(reinterpret_cast<char *>(cart.chr.data()),
              static_cast<std::streamsize>(chr_size));
    if (file.gcount() != static_cast<std::streamsize>(chr_size)) {
      return std::nullopt;
    }
  }

  return cart;
}
