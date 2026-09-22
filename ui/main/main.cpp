#include "nes/nes.h"
#include "window/window.h"

#include <QApplication>

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
  Window window(std::move(nes));
  window.show();
  return app.exec();
}
