#include "bindings.h"

#include "controller/controller.h"

#include <QSettings>

namespace {

struct ButtonDef {
  const char *name;
  uint8_t bit;
  int key;
};

const ButtonDef kButtons[Bindings::kCount] = {
  {"Up", Controller::Up, Qt::Key_W},
  {"Down", Controller::Down, Qt::Key_S},
  {"Left", Controller::Left, Qt::Key_A},
  {"Right", Controller::Right, Qt::Key_D},
  {"A", Controller::A, Qt::Key_L},
  {"B", Controller::B, Qt::Key_K},
  {"Select", Controller::Select, Qt::Key_Shift},
  {"Start", Controller::Start, Qt::Key_Return},
};

} // namespace

Bindings::Bindings() {
  apply_defaults();
  load();
}

uint8_t Bindings::button_for(int key) const {
  if (key == 0) {
    return 0;
  }
  for (int i = 0; i < kCount; ++i) {
    if (keys[i] == key) {
      return kButtons[i].bit;
    }
  }
  return 0;
}

const char *Bindings::name(int index) const { return kButtons[index].name; }

void Bindings::set_key(int index, int key) {
  if (key != 0) {
    for (int i = 0; i < kCount; ++i) {
      if (keys[i] == key) {
        keys[i] = 0;
      }
    }
  }
  keys[index] = key;
  save();
}

void Bindings::reset() {
  apply_defaults();
  save();
}

void Bindings::apply_defaults() {
  for (int i = 0; i < kCount; ++i) {
    keys[i] = kButtons[i].key;
  }
}

void Bindings::load() {
  QSettings store(QStringLiteral("xnes"), QStringLiteral("xnes"));
  store.beginGroup(QStringLiteral("controller"));
  for (int i = 0; i < kCount; ++i) {
    const QString name = QString::fromUtf8(kButtons[i].name);
    if (!store.contains(name)) {
      continue;
    }
    const QVariant value = store.value(name);
    if (value.userType() == QMetaType::QVariantList) {
      continue;
    }
    keys[i] = value.toInt();
  }
}

void Bindings::save() const {
  QSettings store(QStringLiteral("xnes"), QStringLiteral("xnes"));
  store.beginGroup(QStringLiteral("controller"));
  for (int i = 0; i < kCount; ++i) {
    store.setValue(QString::fromUtf8(kButtons[i].name), keys[i]);
  }
}
