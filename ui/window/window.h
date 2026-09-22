#ifndef WINDOW_H
#define WINDOW_H

#include "game_settings/bindings.h"
#include "nes/nes.h"
#include "trace/trace.h"

#include <QMainWindow>
#include <QString>

#include <memory>

class ControllerSettings;
class DebugLog;
class GameView;

class Window : public QMainWindow {
public:
  explicit Window(std::unique_ptr<NES> nes, const QString &rom = {},
                  QWidget *parent = nullptr);

protected:
  void showEvent(QShowEvent *event) override;
  void timerEvent(QTimerEvent *event) override;

private:
  void load_rom();
  void set_rom_title(const QString &path);
  void open_controller();
  void open_debug();

  Bindings bindings;
  std::unique_ptr<NES> nes;
  GameView *game = nullptr;
  ControllerSettings *controller = nullptr;
  DebugLog *debug = nullptr;
  Trace trace;
};

#endif
