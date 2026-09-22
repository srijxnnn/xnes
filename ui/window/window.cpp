#include "window.h"

#include "debug_log/debug_log.h"
#include "game_settings/controller_settings.h"
#include "game_view/game_view.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QShowEvent>
#include <QToolBar>
#include <QToolButton>

namespace {
constexpr int kScale = 3;
} // namespace

Window::Window(std::unique_ptr<NES> nes, const QString &rom, QWidget *parent)
    : QMainWindow(parent), nes(std::move(nes)) {
  game = new GameView(this->nes.get(), &bindings, this);
  setCentralWidget(game);

  QToolBar *bar = addToolBar(QStringLiteral("Main"));
  bar->setMovable(false);

  QMenu *file_menu = new QMenu(bar);
  QAction *load = file_menu->addAction(QStringLiteral("Load ROM"));
  connect(load, &QAction::triggered, this, [this] { load_rom(); });
  QToolButton *file = new QToolButton(bar);
  file->setText(QStringLiteral("File"));
  file->setMenu(file_menu);
  file->setPopupMode(QToolButton::InstantPopup);
  file->setAutoRaise(true);
  bar->addWidget(file);

  QMenu *game_menu = new QMenu(bar);
  QAction *controller_settings =
      game_menu->addAction(QStringLiteral("Controller settings"));
  connect(controller_settings, &QAction::triggered, this,
          [this] { open_controller(); });
  QToolButton *game_button = new QToolButton(bar);
  game_button->setText(QStringLiteral("Game"));
  game_button->setMenu(game_menu);
  game_button->setPopupMode(QToolButton::InstantPopup);
  game_button->setAutoRaise(true);
  bar->addWidget(game_button);

  QAction *debug = bar->addAction(QStringLiteral("Debug"));
  connect(debug, &QAction::triggered, this, [this] { open_debug(); });

  setWindowTitle(QStringLiteral("XNES"));
  if (!rom.isEmpty()) {
    set_rom_title(rom);
  }
  resize(PPU::kWidth * kScale,
         PPU::kHeight * kScale + bar->sizeHint().height());
  startTimer(16);
}

void Window::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
  game->setFocus();
}

void Window::load_rom() {
  const QString path =
      QFileDialog::getOpenFileName(this, QStringLiteral("Load ROM"), QString(),
                                   QStringLiteral("iNES ROM (*.nes)"));
  if (path.isEmpty()) {
    game->setFocus();
    return;
  }

  auto loaded = NES::load(path.toStdString());
  if (!loaded) {
    QMessageBox::warning(this, QStringLiteral("XNES"),
                         QStringLiteral("Failed to load ROM."));
    game->setFocus();
    return;
  }

  trace.clear();
  nes = std::move(loaded);
  game->set_nes(nes.get());
  if (debug) {
    debug->set_text("");
  }
  set_rom_title(path);
  game->update();
  game->setFocus();
}

void Window::set_rom_title(const QString &path) {
  setWindowTitle(QFileInfo(path).fileName() + QStringLiteral(" - XNES"));
}

void Window::open_controller() {
  if (controller) {
    controller->raise();
    controller->activateWindow();
    return;
  }

  controller = new ControllerSettings(&bindings, game, this);
  connect(controller, &QObject::destroyed, this, [this] {
    controller = nullptr;
    game->setFocus();
  });
  controller->show();
}

void Window::open_debug() {
  if (debug) {
    debug->raise();
    debug->activateWindow();
    game->setFocus();
    return;
  }

  trace.clear();
  debug = new DebugLog(this);
  connect(debug, &QObject::destroyed, this, [this] { debug = nullptr; });
  debug->show();
  game->setFocus();
}

void Window::timerEvent(QTimerEvent *) {
  if (!nes) {
    return;
  }
  nes->set_buttons(game->buttons);
  if (debug) {
    const uint64_t frame = nes->frame();
    while (nes->frame() == frame && !nes->halted()) {
      trace.record(nes->cpu);
      nes->step();
    }
    debug->set_text(trace.text());
  } else {
    nes->step_frame();
  }
  game->update();
}
