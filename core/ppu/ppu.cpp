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

} // namespace

void PPU::reset() {
  ctrl_ = 0;
  mask_ = 0;
  status_ = 0;
  oam_addr_ = 0;
  data_buffer_ = 0;
  fine_x_ = 0;
  w_ = false;
  nmi_ = false;
  v_ = 0;
  t_ = 0;
  cycle_ = 0;
  scanline_ = kPreRenderLine;
  sprite_count_ = 0;
  sprite0_cycle_ = -1;
  oam_.fill(0);
  pixels_.fill(kRgb[0]);
}

bool PPU::take_nmi() {
  if (!nmi_) {
    return false;
  }
  nmi_ = false;
  return true;
}

void PPU::increment_x_(uint16_t &v) const {
  if ((v & 0x001F) == 31) {
    v &= ~0x001F;
    v ^= 0x0400;
  } else {
    v += 1;
  }
}

void PPU::increment_y_() {
  if ((v_ & 0x7000) != 0x7000) {
    v_ += 0x1000;
    return;
  }
  v_ &= ~0x7000;
  uint16_t y = (v_ & 0x03E0) >> 5;
  if (y == 29) {
    y = 0;
    v_ ^= 0x0800;
  } else if (y == 31) {
    y = 0;
  } else {
    y += 1;
  }
  v_ = (v_ & ~0x03E0) | (y << 5);
}

void PPU::copy_x_() { v_ = (v_ & ~0x041F) | (t_ & 0x041F); }

void PPU::copy_y_() { v_ = (v_ & ~0x7BE0) | (t_ & 0x7BE0); }

uint16_t PPU::sprite_pattern_addr_(uint8_t tile, int row) const {
  uint16_t table;
  if (ctrl_ & TallSprites) {
    table = static_cast<uint16_t>(tile & 1) << 12;
    tile &= 0xFE;
    if (row >= 8) {
      tile |= 1;
      row -= 8;
    }
  } else {
    table = (ctrl_ & SpritePatternHigh) ? 0x1000 : 0;
  }
  return table | (static_cast<uint16_t>(tile) << 4) |
         static_cast<uint16_t>(row);
}

// Collects the sprites covering `scanline` and fetches the one pattern row
// each of them needs, front to back in OAM order.
void PPU::eval_sprites_(int scanline) {
  sprite_count_ = 0;
  const int height = (ctrl_ & TallSprites) ? 16 : 8;

  for (int i = 0; i < 64; i++) {
    const uint8_t *entry = &oam_[i * 4]; // y - 1, tile, attributes, x
    const int row = scanline - (static_cast<int>(entry[0]) + 1);
    if (row < 0 || row >= height) {
      continue;
    }
    if (sprite_count_ == kSpritesPerLine) {
      status_ |= SpriteOverflow;
      break;
    }

    const int line = (entry[2] & AttrFlipY) ? height - 1 - row : row;
    const uint16_t addr = sprite_pattern_addr_(entry[1], line);

    Sprite &sprite = sprites_[sprite_count_++];
    sprite.x = entry[3];
    sprite.lo = bus_.pattern(addr);
    sprite.hi = bus_.pattern(addr + 8);
    sprite.attr = entry[2];
    sprite.index = static_cast<uint8_t>(i);
  }
}

// Fetches the tile and attribute `v` points at and returns the pixel `fine_x`
// selects from it. A real PPU pipelines these reads a tile ahead; doing them
// per pixel gives the same result because nothing changes mid-scanline.
PPU::Dot PPU::background_dot_(uint16_t v, int fine_x) {
  const uint8_t tile = bus_.read(0x2000 | (v & 0x0FFF));
  const uint8_t attr = bus_.read(0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) |
                                 ((v >> 2) & 0x07));
  const uint16_t pattern = ((ctrl_ & BgPatternHigh) ? 0x1000 : 0) |
                           (static_cast<uint16_t>(tile) << 4) |
                           ((v >> 12) & 7);
  const uint8_t lo = bus_.pattern(pattern);
  const uint8_t hi = bus_.pattern(pattern + 8);
  const int bit = 7 - fine_x;

  Dot dot;
  dot.value = static_cast<uint8_t>(((hi >> bit) & 1) << 1 | ((lo >> bit) & 1));
  // One attribute byte covers four tiles; bits 1 and 6 of v pick the quadrant.
  dot.palette = (attr >> (((v >> 4) & 4) | (v & 2))) & AttrPalette;
  return dot;
}

// Walks the scanline's sprites back to front so the last opaque one written is
// the frontmost, which for overlapping sprites is the lowest OAM index.
PPU::SpriteDot PPU::sprite_dot_(int x) const {
  SpriteDot dot;
  for (int i = sprite_count_ - 1; i >= 0; i--) {
    const Sprite &sprite = sprites_[i];
    const int offset = x - sprite.x;
    if (offset < 0 || offset > 7) {
      continue;
    }

    const int bit = (sprite.attr & AttrFlipX) ? offset : (7 - offset);
    const uint8_t value = static_cast<uint8_t>(
        ((sprite.hi >> bit) & 1) << 1 | ((sprite.lo >> bit) & 1));
    if (value) {
      dot.value = value;
      dot.palette = sprite.attr & AttrPalette;
      dot.behind_bg = sprite.attr & AttrBehindBg;
      dot.is_sprite0 = sprite.index == 0;
    }
  }
  return dot;
}

uint8_t PPU::colour_of_(const Dot &bg, const SpriteDot &sp) const {
  if (sp.value && (bg.value == 0 || !sp.behind_bg)) {
    return bus_.colour(0x10 | (sp.palette << 2) | sp.value);
  }
  if (bg.value) {
    return bus_.colour((bg.palette << 2) | bg.value);
  }
  return bus_.colour(0);
}

void PPU::render_scanline_(int y) {
  sprite0_cycle_ = -1;

  if (!rendering_()) {
    const uint32_t backdrop = kRgb[bus_.colour(0)];
    for (int x = 0; x < kWidth; x++) {
      pixels_[y * kWidth + x] = backdrop;
    }
    return;
  }

  eval_sprites_(y);

  // Local copies: the scanline is drawn in one go, so advancing the real v
  // here would run ahead of the dot counter in tick().
  uint16_t v = v_;
  int fine_x = fine_x_;

  for (int x = 0; x < kWidth; x++) {
    // Both layers can be hidden in the leftmost 8 pixels, which games use to
    // cover the column that scrolling drags in.
    Dot bg;
    if ((mask_ & ShowBg) && (x >= 8 || (mask_ & ShowBgLeft))) {
      bg = background_dot_(v, fine_x);
    }

    SpriteDot sp;
    if ((mask_ & ShowSprites) && (x >= 8 || (mask_ & ShowSpritesLeft))) {
      sp = sprite_dot_(x);
    }

    // The hit is reported a dot later, and never on the last pixel.
    if (sp.is_sprite0 && bg.value && x != 255 && sprite0_cycle_ < 0) {
      sprite0_cycle_ = x + 1;
    }

    pixels_[y * kWidth + x] = kRgb[colour_of_(bg, sp)];

    if (++fine_x == 8) {
      fine_x = 0;
      increment_x_(v);
    }
  }
}

void PPU::tick() {
  const bool visible = scanline_ < kHeight;

  if (visible && cycle_ == 1) {
    render_scanline_(scanline_);
  }

  // render_scanline_ works out where sprite 0 was hit; the flag is only raised
  // once the dot counter reaches it, because games poll for exactly that dot.
  if (visible && sprite0_cycle_ >= 0 && cycle_ == sprite0_cycle_) {
    status_ |= Sprite0Hit;
    sprite0_cycle_ = -1;
  }

  if (scanline_ == kVBlankLine && cycle_ == 1) {
    status_ |= VBlank;
    if (ctrl_ & NmiEnable) {
      nmi_ = true;
    }
  }

  if (scanline_ == kPreRenderLine && cycle_ == 1) {
    status_ &= ~(VBlank | Sprite0Hit | SpriteOverflow);
    nmi_ = false;
  }

  // Scroll reloads happen at fixed dots, which is what makes a mid-frame $2005
  // write land on the next scanline rather than this one.
  if (rendering_() && (visible || scanline_ == kPreRenderLine)) {
    if (cycle_ == 256) {
      increment_y_();
    }
    if (cycle_ == 257) {
      copy_x_();
    }
    if (cycle_ == 280 && scanline_ == kPreRenderLine) {
      copy_y_();
    }
  }

  if (++cycle_ == kDotsPerLine) {
    cycle_ = 0;
    if (++scanline_ > kPreRenderLine) {
      scanline_ = 0;
      frame_++;
    }
  }
}

uint8_t PPU::cpu_read(uint16_t addr) {
  switch (addr & 7) {
  // Reading the status register clears vblank and resets the $2005/$2006 write
  // latch. The unused low bits read back as stale bus data.
  case 2: {
    const uint8_t result = (status_ & (VBlank | Sprite0Hit | SpriteOverflow)) |
                           (data_buffer_ & 0x1F);
    status_ &= ~VBlank;
    w_ = false;
    return result;
  }
  case 4:
    return oam_[oam_addr_];
  // $2007 reads lag one byte behind, except in palette memory, which is not
  // buffered.
  case 7: {
    uint8_t value = data_buffer_;
    data_buffer_ = bus_.read(v_);
    if ((v_ & 0x3FFF) >= 0x3F00) {
      value = data_buffer_;
      data_buffer_ = bus_.read(v_ - 0x1000);
    }
    v_ += (ctrl_ & AddrStep32) ? 32 : 1;
    return value;
  }
  default:
    return data_buffer_;
  }
}

void PPU::cpu_write(uint16_t addr, uint8_t data) {
  data_buffer_ = data;
  switch (addr & 7) {
  // Enabling NMI while vblank is already up fires one immediately.
  case 0: {
    const bool was_enabled = ctrl_ & NmiEnable;
    ctrl_ = data;
    t_ = (t_ & 0xF3FF) |
         (static_cast<uint16_t>(data & NametableSelect) << 10);
    if (!was_enabled && (ctrl_ & NmiEnable) && (status_ & VBlank)) {
      nmi_ = true;
    }
    break;
  }
  case 1:
    mask_ = data;
    break;
  case 3:
    oam_addr_ = data;
    break;
  case 4:
    oam_[oam_addr_++] = data;
    break;
  case 5:
    if (!w_) {
      t_ = (t_ & 0xFFE0) | (data >> 3);
      fine_x_ = data & 0x07;
      w_ = true;
    } else {
      t_ = (t_ & 0x8FFF) | (static_cast<uint16_t>(data & 0x07) << 12);
      t_ = (t_ & 0xFC1F) | (static_cast<uint16_t>(data & 0xF8) << 2);
      w_ = false;
    }
    break;
  case 6:
    if (!w_) {
      t_ = (t_ & 0x00FF) | (static_cast<uint16_t>(data & 0x3F) << 8);
      w_ = true;
    } else {
      t_ = (t_ & 0xFF00) | data;
      v_ = t_;
      w_ = false;
    }
    break;
  case 7:
    bus_.write(v_, data);
    v_ += (ctrl_ & AddrStep32) ? 32 : 1;
    break;
  }
}

void PPU::oam_write(uint8_t data) { oam_[oam_addr_++] = data; }
