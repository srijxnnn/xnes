#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include "nes/nes.h"

#include <QWidget>

class GameView : public QWidget {
public:
  explicit GameView(NES *nes, QWidget *parent = nullptr);

  void set_nes(NES *nes);
  uint8_t buttons = 0;

protected:
  void paintEvent(QPaintEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;

private:
  static uint8_t button_for(int key);

  NES *nes;
};

#endif
