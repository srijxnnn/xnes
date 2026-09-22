#include "game_view.h"

#include "controller/controller.h"

#include <QKeyEvent>
#include <QPainter>

#include <algorithm>

GameView::GameView(NES *nes, QWidget *parent) : QWidget(parent), nes(nes) {
  setFocusPolicy(Qt::StrongFocus);
  setMinimumSize(PPU::kWidth, PPU::kHeight);
}

void GameView::set_nes(NES *nes) {
  this->nes = nes;
  buttons = 0;
}

uint8_t GameView::button_for(int key) {
  switch (key) {
  case Qt::Key_Z:
    return Controller::A;
  case Qt::Key_X:
    return Controller::B;
  case Qt::Key_Shift:
    return Controller::Select;
  case Qt::Key_Return:
  case Qt::Key_Enter:
    return Controller::Start;
  case Qt::Key_Up:
    return Controller::Up;
  case Qt::Key_Down:
    return Controller::Down;
  case Qt::Key_Left:
    return Controller::Left;
  case Qt::Key_Right:
    return Controller::Right;
  default:
    return 0;
  }
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
  buttons |= button_for(event->key());
}

void GameView::keyReleaseEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) {
    return;
  }
  buttons &= static_cast<uint8_t>(~button_for(event->key()));
}
