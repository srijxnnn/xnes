#ifndef WINDOW_H
#define WINDOW_H

#include "nes/nes.h"

#include <QWidget>

#include <memory>

class Window : public QWidget {
public:
  explicit Window(std::unique_ptr<NES> nes, QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void timerEvent(QTimerEvent *event) override;

private:
  static uint8_t button_for_(int key);

  std::unique_ptr<NES> nes_;
  uint8_t buttons_ = 0;
};

#endif
