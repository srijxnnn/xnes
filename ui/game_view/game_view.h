#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include "nes/nes.h"

#include <QSet>
#include <QWidget>

class Bindings;

class GameView : public QWidget {
public:
  explicit GameView(NES *nes, Bindings *bindings, QWidget *parent = nullptr);

  void set_nes(NES *nes);
  void refresh_buttons();
  uint8_t buttons = 0;

protected:
  void paintEvent(QPaintEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;

private:
  NES *nes;
  Bindings *bindings;
  QSet<int> held;
};

#endif
