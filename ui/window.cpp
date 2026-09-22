#include "window.h"

#include "controller/controller.h"

#include <QKeyEvent>
#include <QPainter>

#include <algorithm>

namespace {
constexpr int kScale = 3;
}

Window::Window(std::unique_ptr<NES> nes, QWidget *parent)
    : QWidget(parent), nes_(std::move(nes)) {
  setWindowTitle("XNES");
  setMinimumSize(PPU::kWidth, PPU::kHeight);
  resize(PPU::kWidth * kScale, PPU::kHeight * kScale);
  setFocusPolicy(Qt::StrongFocus);
  startTimer(16);
}

void Window::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
  painter.fillRect(rect(), Qt::black);

  const QImage frame(reinterpret_cast<const uchar *>(nes_->pixels()),
                     PPU::kWidth, PPU::kHeight, PPU::kWidth * 4,
                     QImage::Format_ARGB32);

  const int scale = std::max(1, std::min(width() / PPU::kWidth,
                                         height() / PPU::kHeight));
  const int w = PPU::kWidth * scale;
  const int h = PPU::kHeight * scale;
  painter.drawImage(QRect((width() - w) / 2, (height() - h) / 2, w, h), frame);
}

void Window::timerEvent(QTimerEvent *) {
  nes_->set_buttons(buttons_);
  nes_->step_frame();
  update();
}

uint8_t Window::button_for_(int key) {
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

void Window::keyPressEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) {
    return;
  }
  if (event->key() == Qt::Key_Escape) {
    close();
    return;
  }
  if (event->key() == Qt::Key_F5) {
    nes_->reset();
    return;
  }
  buttons_ |= button_for_(event->key());
}

void Window::keyReleaseEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) {
    return;
  }
  buttons_ &= static_cast<uint8_t>(~button_for_(event->key()));
}
