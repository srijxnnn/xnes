#include "nes/nes.h"
#include "window/window.h"

#include <QApplication>
#include <QFile>
#include <QIcon>

#include <cstdio>
#include <memory>

int main(int argc, char *argv[]) {
  std::unique_ptr<NES> nes;
  if (argc >= 2) {
    nes = NES::load(argv[1]);
    if (!nes) {
      std::fprintf(stderr, "failed to load ROM: %s\n", argv[1]);
      return 1;
    }
  }

  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(QStringLiteral(":/logo.png")));
  const QString rom = argc >= 2 ? QFile::decodeName(argv[1]) : QString();
  Window window(std::move(nes), rom);
  window.show();
  return app.exec();
}
