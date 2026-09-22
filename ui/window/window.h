#ifndef WINDOW_H
#define WINDOW_H

#include "nes/nes.h"
#include "trace/trace.h"

#include <QMainWindow>

#include <memory>

class DebugLog;
class GameView;

class Window : public QMainWindow {
public:
  explicit Window(std::unique_ptr<NES> nes, QWidget *parent = nullptr);

protected:
  void showEvent(QShowEvent *event) override;
  void timerEvent(QTimerEvent *event) override;

private:
  void load_rom();
  void open_debug();

  std::unique_ptr<NES> nes;
  GameView *game = nullptr;
  DebugLog *debug = nullptr;
  Trace trace;
};

#endif
