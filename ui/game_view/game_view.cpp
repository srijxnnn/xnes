#include "game_view.h"

#include "game_settings/bindings.h"

#include <QKeyEvent>
#include <QPainter>

#include <algorithm>

GameView::GameView(NES *nes, Bindings *bindings, QWidget *parent)
    : QWidget(parent), nes(nes), bindings(bindings) {
  setFocusPolicy(Qt::StrongFocus);
  setMinimumSize(PPU::kWidth, PPU::kHeight);
}

void GameView::set_nes(NES *nes) {
  this->nes = nes;
  held.clear();
  buttons = 0;
}

void GameView::refresh_buttons() {
  uint8_t mask = 0;
  for (int key : held) {
    mask |= bindings->button_for(key);
  }
  buttons = mask;
}

void GameView::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
  painter.fillRect(rect(), Qt::black);
  if (!nes) {
    return;
  }

  const QImage frame(reinterpret_cast<const uchar *>(nes->pixels()),
                     PPU::kWidth, PPU::kHeight, PPU::kWidth * 4,
                     QImage::Format_ARGB32);

  const int scale =
      std::max(1, std::min(width() / PPU::kWidth, height() / PPU::kHeight));
  const int w = PPU::kWidth * scale;
  const int h = PPU::kHeight * scale;
  painter.drawImage(QRect((width() - w) / 2, (height() - h) / 2, w, h), frame);
}

void GameView::keyPressEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) {
    return;
  }
  if (event->key() == Qt::Key_Escape) {
    window()->close();
    return;
  }
  if (event->key() == Qt::Key_F5) {
    if (nes) {
      nes->reset();
    }
    return;
  }
  held.insert(event->key());
  refresh_buttons();
}

void GameView::keyReleaseEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) {
    return;
  }
  held.remove(event->key());
  refresh_buttons();
}
