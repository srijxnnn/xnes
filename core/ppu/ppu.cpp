#include "ppu.h"

namespace {

constexpr uint32_t kRgb[64] = {
    0xFF626262, 0xFF001FB2, 0xFF2404C8, 0xFF5200B2, 0xFF730076, 0xFF800024,
    0xFF730B00, 0xFF522800, 0xFF244400, 0xFF005700, 0xFF005C00, 0xFF005324,
    0xFF003C76, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFABABAB, 0xFF0D57FF,
    0xFF4B30FF, 0xFF8A13FF, 0xFFBC08D6, 0xFFCF0C69, 0xFFC02B00, 0xFF954F00,
    0xFF5F7200, 0xFF28A000, 0xFF00A800, 0xFF00A347, 0xFF007DB4, 0xFF000000,
    0xFF000000, 0xFF000000, 0xFFFFFFFF, 0xFF53AEFF, 0xFF9085FF, 0xFFD365FF,
    0xFFFF57FF, 0xFFFF5DCC, 0xFFFF7757, 0xFFFA9E00, 0xFFC7C700, 0xFF8FE800,
    0xFF53F83F, 0xFF1BF19F, 0xFF33D6FF, 0xFF4E4E4E, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFFB6E1FF, 0xFFCED1FF, 0xFFE9C3FF, 0xFFFFBCFF, 0xFFFFBDF4,
    0xFFFFC6C3, 0xFFFFD59A, 0xFFE9E894, 0xFFCFEF96, 0xFFB6F4B0, 0xFFB5F1D8,
    0xFFB6EFFF, 0xFFB8B8B8, 0xFF000000, 0xFF000000,
};

}

void PPU::reset() {
  ctrl = 0;
  mask = 0;
  status = 0;
  oam_addr = 0;
  data_buffer = 0;
  fine_x = 0;
  w = false;
  nmi = false;
  v = 0;
  t = 0;
  cycle = 0;
  scanline = kPreRenderLine;
  sprite_count = 0;
  sprite0_cycle = -1;
  oam.fill(0);
  pixels.fill(kRgb[0]);
}

bool PPU::take_nmi() {
  if (!nmi) {
    return false;
  }
  nmi = false;
  return true;
}

void PPU::increment_x(uint16_t &v) const {
  if ((v & 0x001F) == 31) {
    v &= ~0x001F;
    v ^= 0x0400;
  } else {
    v += 1;
  }
}

void PPU::increment_y() {
  if ((v & 0x7000) != 0x7000) {
    v += 0x1000;
    return;
  }
  v &= ~0x7000;
  uint16_t y = (v & 0x03E0) >> 5;
  if (y == 29) {
    y = 0;
    v ^= 0x0800;
  } else if (y == 31) {
    y = 0;
  } else {
    y += 1;
  }
  v = (v & ~0x03E0) | (y << 5);
}

void PPU::copy_x() { v = (v & ~0x041F) | (t & 0x041F); }

void PPU::copy_y() { v = (v & ~0x7BE0) | (t & 0x7BE0); }

uint16_t PPU::sprite_pattern_addr(uint8_t tile, int row) const {
  uint16_t table;
  if (ctrl & TallSprites) {
    table = static_cast<uint16_t>(tile & 1) << 12;
    tile &= 0xFE;
    if (row >= 8) {
      tile |= 1;
      row -= 8;
    }
  } else {
    table = (ctrl & SpritePatternHigh) ? 0x1000 : 0;
  }
  return table | (static_cast<uint16_t>(tile) << 4) |
         static_cast<uint16_t>(row);
}

void PPU::eval_sprites(int scanline) {
  sprite_count = 0;
  const int height = (ctrl & TallSprites) ? 16 : 8;

  for (int i = 0; i < 64; i++) {
    const uint8_t *entry = &oam[i * 4];
    const int row = scanline - (static_cast<int>(entry[0]) + 1);
    if (row < 0 || row >= height) {
      continue;
    }
    if (sprite_count == kSpritesPerLine) {
      status |= SpriteOverflow;
      break;
    }

    const int line = (entry[2] & AttrFlipY) ? height - 1 - row : row;
    const uint16_t addr = sprite_pattern_addr(entry[1], line);

    Sprite &sprite = sprites[sprite_count++];
    sprite.x = entry[3];
    sprite.lo = bus.pattern(addr);
    sprite.hi = bus.pattern(addr + 8);
    sprite.attr = entry[2];
    sprite.index = static_cast<uint8_t>(i);
  }
}

PPU::Dot PPU::background_dot(uint16_t v, int fine_x) {
  const uint8_t tile = bus.read(0x2000 | (v & 0x0FFF));
  const uint8_t attr =
      bus.read(0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07));
  const uint16_t pattern = ((ctrl & BgPatternHigh) ? 0x1000 : 0) |
                           (static_cast<uint16_t>(tile) << 4) | ((v >> 12) & 7);
  const uint8_t lo = bus.pattern(pattern);
  const uint8_t hi = bus.pattern(pattern + 8);
  const int bit = 7 - fine_x;

  Dot dot;
  dot.value = static_cast<uint8_t>(((hi >> bit) & 1) << 1 | ((lo >> bit) & 1));
  dot.palette = (attr >> (((v >> 4) & 4) | (v & 2))) & AttrPalette;
  return dot;
}

PPU::SpriteDot PPU::sprite_dot(int x) const {
  SpriteDot dot;
  for (int i = sprite_count - 1; i >= 0; i--) {
    const Sprite &sprite = sprites[i];
    const int offset = x - sprite.x;
    if (offset < 0 || offset > 7) {
      continue;
    }

    const int bit = (sprite.attr & AttrFlipX) ? offset : (7 - offset);
    const uint8_t value = static_cast<uint8_t>(((sprite.hi >> bit) & 1) << 1 |
                                               ((sprite.lo >> bit) & 1));
    if (value) {
      dot.value = value;
      dot.palette = sprite.attr & AttrPalette;
      dot.behind_bg = sprite.attr & AttrBehindBg;
      dot.is_sprite0 = sprite.index == 0;
    }
  }
  return dot;
}

uint8_t PPU::colour_of(const Dot &bg, const SpriteDot &sp) const {
  if (sp.value && (bg.value == 0 || !sp.behind_bg)) {
    return bus.colour(0x10 | (sp.palette << 2) | sp.value);
  }
  if (bg.value) {
    return bus.colour((bg.palette << 2) | bg.value);
  }
  return bus.colour(0);
}

void PPU::render_scanline(int y) {
  sprite0_cycle = -1;

  if (!rendering()) {
    const uint32_t backdrop = kRgb[bus.colour(0)];
    for (int x = 0; x < kWidth; x++) {
      pixels[y * kWidth + x] = backdrop;
    }
    return;
  }

  eval_sprites(y);

  uint16_t v = this->v;
  int fine_x = this->fine_x;

  for (int x = 0; x < kWidth; x++) {
    Dot bg;
    if ((mask & ShowBg) && (x >= 8 || (mask & ShowBgLeft))) {
      bg = background_dot(v, fine_x);
    }

    SpriteDot sp;
    if ((mask & ShowSprites) && (x >= 8 || (mask & ShowSpritesLeft))) {
      sp = sprite_dot(x);
    }

    if (sp.is_sprite0 && bg.value && x != 255 && sprite0_cycle < 0) {
      sprite0_cycle = x + 1;
    }

    pixels[y * kWidth + x] = kRgb[colour_of(bg, sp)];

    if (++fine_x == 8) {
      fine_x = 0;
      increment_x(v);
    }
  }
}

void PPU::tick() {
  const bool visible = scanline < kHeight;

  if (visible && cycle == 1) {
    render_scanline(scanline);
  }

  if (visible && sprite0_cycle >= 0 && cycle == sprite0_cycle) {
    status |= Sprite0Hit;
    sprite0_cycle = -1;
  }

  if (scanline == kVBlankLine && cycle == 1) {
    status |= VBlank;
    if (ctrl & NmiEnable) {
      nmi = true;
    }
  }

  if (scanline == kPreRenderLine && cycle == 1) {
    status &= ~(VBlank | Sprite0Hit | SpriteOverflow);
    nmi = false;
  }

  if (rendering() && (visible || scanline == kPreRenderLine)) {
    if (cycle == 256) {
      increment_y();
    }
    if (cycle == 257) {
      copy_x();
    }
    if (cycle == 280 && scanline == kPreRenderLine) {
      copy_y();
    }
  }

  if (++cycle == kDotsPerLine) {
    cycle = 0;
    if (++scanline > kPreRenderLine) {
      scanline = 0;
      frame++;
    }
  }
}

uint8_t PPU::cpu_read(uint16_t addr) {
  switch (addr & 7) {
  case 2: {
    const uint8_t result = (status & (VBlank | Sprite0Hit | SpriteOverflow)) |
                           (data_buffer & 0x1F);
    status &= ~VBlank;
    w = false;
    return result;
  }
  case 4:
    return oam[oam_addr];
  case 7: {
    uint8_t value = data_buffer;
    data_buffer = bus.read(v);
    if ((v & 0x3FFF) >= 0x3F00) {
      value = data_buffer;
      data_buffer = bus.read(v - 0x1000);
    }
    v += (ctrl & AddrStep32) ? 32 : 1;
    return value;
  }
  default:
    return data_buffer;
  }
}

void PPU::cpu_write(uint16_t addr, uint8_t data) {
  data_buffer = data;
  switch (addr & 7) {
  case 0: {
    const bool was_enabled = ctrl & NmiEnable;
    ctrl = data;
    t = (t & 0xF3FF) | (static_cast<uint16_t>(data & NametableSelect) << 10);
    if (!was_enabled && (ctrl & NmiEnable) && (status & VBlank)) {
      nmi = true;
    }
    break;
  }
  case 1:
    mask = data;
    break;
  case 3:
    oam_addr = data;
    break;
  case 4:
    oam[oam_addr++] = data;
    break;
  case 5:
    if (!w) {
      t = (t & 0xFFE0) | (data >> 3);
      fine_x = data & 0x07;
      w = true;
    } else {
      t = (t & 0x8FFF) | (static_cast<uint16_t>(data & 0x07) << 12);
      t = (t & 0xFC1F) | (static_cast<uint16_t>(data & 0xF8) << 2);
      w = false;
    }
    break;
  case 6:
    if (!w) {
      t = (t & 0x00FF) | (static_cast<uint16_t>(data & 0x3F) << 8);
      w = true;
    } else {
      t = (t & 0xFF00) | data;
      v = t;
      w = false;
    }
    break;
  case 7:
    bus.write(v, data);
    v += (ctrl & AddrStep32) ? 32 : 1;
    break;
  }
}

void PPU::oam_write(uint8_t data) { oam[oam_addr++] = data; }
