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

  int mapper = header[6] >> 4;
  if ((header[7] & 0x0C) == 0x08) {
    mapper |= (header[7] & 0xF0) | (static_cast<int>(header[8] & 0x0F) << 8);
  } else if ((header[7] & 0x0C) == 0x00) {
    mapper |= header[7] & 0xF0;
  }
  if (mapper != 0) {
    return std::nullopt;
  }

  const std::size_t prg_size = static_cast<std::size_t>(header[4]) * 16384;
  const std::size_t chr_size = static_cast<std::size_t>(header[5]) * 8192;
  if (prg_size == 0) {
    return std::nullopt;
  }

  Cartridge cart;
  if (header[6] & 0x08) {
    cart.mirror_ = Mirror::Four;
  } else if (header[6] & 0x01) {
    cart.mirror_ = Mirror::Vertical;
  } else {
    cart.mirror_ = Mirror::Horizontal;
  }

  if (header[6] & 0x04) {
    file.seekg(512, std::ios::cur);
    if (!file) {
      return std::nullopt;
    }
  }

  cart.prg_.resize(prg_size);
  file.read(reinterpret_cast<char *>(cart.prg_.data()),
            static_cast<std::streamsize>(prg_size));
  if (file.gcount() != static_cast<std::streamsize>(prg_size)) {
    return std::nullopt;
  }

  if (chr_size == 0) {
    cart.chr_.assign(8192, 0);
    cart.chr_ram_ = true;
  } else {
    cart.chr_.resize(chr_size);
    file.read(reinterpret_cast<char *>(cart.chr_.data()),
              static_cast<std::streamsize>(chr_size));
    if (file.gcount() != static_cast<std::streamsize>(chr_size)) {
      return std::nullopt;
    }
  }

  return cart;
}

uint8_t Cartridge::cpu_read(uint16_t addr) const {
  if (addr >= 0x8000) {
    return prg_[(addr - 0x8000) & (prg_.size() - 1)];
  }
  if (addr >= 0x6000) {
    return prg_ram_[addr - 0x6000];
  }
  return 0;
}

void Cartridge::cpu_write(uint16_t addr, uint8_t data) {
  if (addr >= 0x6000 && addr < 0x8000) {
    prg_ram_[addr - 0x6000] = data;
  }
}

uint8_t Cartridge::chr_read(uint16_t addr) const {
  return chr_[addr & (chr_.size() - 1)];
}

void Cartridge::chr_write(uint16_t addr, uint8_t data) {
  if (chr_ram_) {
    chr_[addr & (chr_.size() - 1)] = data;
  }
}
