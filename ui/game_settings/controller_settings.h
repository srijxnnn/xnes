#ifndef CONTROLLER_SETTINGS_H
#define CONTROLLER_SETTINGS_H

#include "bindings.h"

#include <QWidget>

class GameView;
class QPushButton;

class ControllerSettings : public QWidget {
public:
  ControllerSettings(Bindings *bindings, GameView *game,
                     QWidget *parent = nullptr);

protected:
  void keyPressEvent(QKeyEvent *event) override;

private:
  void begin_capture(int index);
  void assign(int key);
  void refresh();

  Bindings *bindings;
  GameView *game;
  QPushButton *key_buttons[Bindings::kCount]{};
  int capture_index = -1;
};

#endif
