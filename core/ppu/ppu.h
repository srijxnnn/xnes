#ifndef PPU_H
#define PPU_H

#include "bus/ppu_bus.h"

#include <array>
#include <cstdint>

// Scanline PPU: the registers at $2000-$2007, the sprite and scroll state, and
// the renderer. Its memory lives behind PpuBus. Dots are counted so NMI and
// sprite-0 land on the right cycle; pixels for a scanline are produced from the
// registers at the start of that line, which is enough for Donkey Kong's
// status-bar split.
class PPU {
public:
  static constexpr int kWidth = 256;
  static constexpr int kHeight = 240;

  explicit PPU(PpuBus &bus) : bus(bus) {}

  void reset();
  void tick();

  uint8_t cpu_read(uint16_t addr);
  void cpu_write(uint16_t addr, uint8_t data);
  void oam_write(uint8_t data);

  bool take_nmi();
  uint64_t frame = 0;
  std::array<uint32_t, kWidth * kHeight> pixels{};

private:
  static constexpr int kSpritesPerLine = 8;
  static constexpr int kDotsPerLine = 341;
  static constexpr int kVBlankLine = 241;
  static constexpr int kPreRenderLine = 261;

  enum Ctrl : uint8_t { // $2000
    NametableSelect = 0x03,
    AddrStep32 = 0x04,
    SpritePatternHigh = 0x08,
    BgPatternHigh = 0x10,
    TallSprites = 0x20,
    NmiEnable = 0x80,
  };

  enum Mask : uint8_t { // $2001
    ShowBgLeft = 0x02,
    ShowSpritesLeft = 0x04,
    ShowBg = 0x08,
    ShowSprites = 0x10,
  };

  enum Status : uint8_t { // $2002
    SpriteOverflow = 0x20,
    Sprite0Hit = 0x40,
    VBlank = 0x80,
  };

  enum Attr : uint8_t { // OAM byte 2
    AttrPalette = 0x03,
    AttrBehindBg = 0x20,
    AttrFlipX = 0x40,
    AttrFlipY = 0x80,
  };

  // A sprite already fetched for the current scanline: its pattern bits are
  // for this line only, so rendering is a shift instead of another CHR read.
  struct Sprite {
    uint8_t x = 0;
    uint8_t lo = 0;
    uint8_t hi = 0;
    uint8_t attr = 0;
    uint8_t index = 0;
  };

  // One pattern pixel: a 2-bit colour within `palette`, where 0 is transparent.
  struct Dot {
    uint8_t value = 0;
    uint8_t palette = 0;
  };

  struct SpriteDot : Dot {
    bool behind_bg = false;
    bool is_sprite0 = false;
  };

  PpuBus &bus;

  std::array<uint8_t, 256> oam{};
  std::array<Sprite, kSpritesPerLine> sprites{};

  uint8_t ctrl = 0;
  uint8_t mask = 0;
  uint8_t status = 0;
  uint8_t oam_addr = 0;
  uint8_t data_buffer = 0;
  uint8_t fine_x = 0;
  bool w = false;
  bool nmi = false;

  uint16_t v = 0;
  uint16_t t = 0;

  int cycle = 0;
  int scanline = kPreRenderLine;
  int sprite_count = 0;
  int sprite0_cycle = -1;

  bool rendering() const { return (mask & (ShowBg | ShowSprites)) != 0; }

  void increment_x(uint16_t &v) const;
  void increment_y();
  void copy_x();
  void copy_y();

  // CHR address of one pattern row. In 8x16 mode bit 0 of the tile picks the
  // pattern table and rows 8-15 come from the next tile.
  uint16_t sprite_pattern_addr(uint8_t tile, int row) const;

  void eval_sprites(int scanline);
  void render_scanline(int y);

  Dot background_dot(uint16_t v, int fine_x);
  SpriteDot sprite_dot(int x) const;
  uint8_t colour_of(const Dot &bg, const SpriteDot &sp) const;
};

#endif
