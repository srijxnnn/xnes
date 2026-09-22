#include "controller_settings.h"

#include "game_view/game_view.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString key_label(int key) {
  if (key == 0) {
    return QStringLiteral("None");
  }
  const QString text = QKeySequence(key).toString(QKeySequence::NativeText);
  return text.isEmpty() ? QString::number(key) : text;
}

} // namespace

ControllerSettings::ControllerSettings(Bindings *bindings, GameView *game,
                                       QWidget *parent)
    : QWidget(parent), bindings(bindings), game(game) {
  setWindowFlag(Qt::Window);
  setWindowTitle(QStringLiteral("Controller settings"));
  setAttribute(Qt::WA_DeleteOnClose);
  setFocusPolicy(Qt::StrongFocus);

  auto *layout = new QVBoxLayout(this);
  auto *box = new QGroupBox(QStringLiteral("Controller"), this);
  auto *grid = new QGridLayout(box);
  for (int i = 0; i < Bindings::kCount; ++i) {
    grid->addWidget(new QLabel(QString::fromUtf8(bindings->name(i)), box), i,
                    0);
    auto *button = new QPushButton(box);
    button->setFocusPolicy(Qt::NoFocus);
    button->setMinimumWidth(96);
    key_buttons[i] = button;
    grid->addWidget(button, i, 1);
    connect(button, &QPushButton::clicked, this,
            [this, i] { begin_capture(i); });
  }
  layout->addWidget(box);

  auto *hint = new QLabel(
      QStringLiteral("Click a key, then press a new one. Backspace clears it."),
      this);
  hint->setWordWrap(true);
  layout->addWidget(hint);

  auto *reset = new QPushButton(QStringLiteral("Reset"), this);
  reset->setFocusPolicy(Qt::NoFocus);
  connect(reset, &QPushButton::clicked, this, [this] {
    capture_index = -1;
    this->bindings->reset();
    refresh();
    this->game->refresh_buttons();
  });
  layout->addWidget(reset, 0, Qt::AlignRight);

  refresh();
  adjustSize();
}

void ControllerSettings::begin_capture(int index) {
  capture_index = index;
  refresh();
  key_buttons[index]->setText(QStringLiteral("..."));
  setFocus(Qt::OtherFocusReason);
}

void ControllerSettings::assign(int key) {
  bindings->set_key(capture_index, key);
  capture_index = -1;
  refresh();
  game->refresh_buttons();
}

void ControllerSettings::refresh() {
  for (int i = 0; i < Bindings::kCount; ++i) {
    key_buttons[i]->setText(key_label(bindings->key(i)));
  }
}

void ControllerSettings::keyPressEvent(QKeyEvent *event) {
  if (capture_index < 0 || event->isAutoRepeat()) {
    QWidget::keyPressEvent(event);
    return;
  }
  const int key = event->key();
  if (key == Qt::Key_Escape || key == Qt::Key_unknown) {
    capture_index = -1;
    refresh();
    return;
  }
  if (key == Qt::Key_F5) {
    return;
  }
  if (key == Qt::Key_Backspace) {
    assign(0);
    return;
  }
  assign(key);
}
