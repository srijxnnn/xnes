#include "nes/nes.h"
#include "window.h"

#include <QApplication>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace {

std::unique_ptr<NES> load_rom(const char *path) {
  auto nes = NES::load(path);
  if (!nes) {
    std::fprintf(stderr, "failed to load ROM: %s\n", path);
  }
  return nes;
}

// One line per instruction in the format test/run_nestest.sh compares against
// the golden log.
void trace(const CPU &cpu) {
  std::printf("%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%llu\n", cpu.pc(),
              cpu.a(), cpu.x(), cpu.y(), cpu.p(), cpu.sp(),
              static_cast<unsigned long long>(cpu.cycles()));
}

void print_usage(const char *argv0) {
  std::fprintf(stderr,
               "usage: %s <rom.nes>\n"
               "       %s --nestest <rom.nes>\n"
               "       %s --dump-ppm <frames> <out.ppm> <rom.nes>\n",
               argv0, argv0, argv0);
}

void write_ppm(const char *path, const uint32_t *pixels) {
  FILE *out = std::fopen(path, "wb");
  if (!out) {
    std::fprintf(stderr, "failed to write %s\n", path);
    return;
  }
  std::fprintf(out, "P6\n%d %d\n255\n", PPU::kWidth, PPU::kHeight);
  for (int i = 0; i < PPU::kWidth * PPU::kHeight; i++) {
    const uint32_t c = pixels[i];
    const uint8_t rgb[3] = {static_cast<uint8_t>(c >> 16),
                            static_cast<uint8_t>(c >> 8),
                            static_cast<uint8_t>(c)};
    std::fwrite(rgb, 1, 3, out);
  }
  std::fclose(out);
}

int run_dump(int frames, const char *ppm, const char *rom) {
  auto nes = load_rom(rom);
  if (!nes) {
    return 1;
  }
  for (int i = 0; i < frames && !nes->halted(); i++) {
    nes->step_frame();
  }
  write_ppm(ppm, nes->pixels());
  return 0;
}

int run_nestest(const char *path) {
  auto nes = load_rom(path);
  if (!nes) {
    return 1;
  }

  // nestest's automated mode starts here instead of at the reset vector.
  nes->cpu().set_pc(0xC000);

  while (!nes->halted()) {
    trace(nes->cpu());
    nes->cpu().step();
  }
  return 0;
}

int run_game(int argc, char *argv[], const char *path) {
  auto nes = load_rom(path);
  if (!nes) {
    return 1;
  }

  QApplication app(argc, argv);
  Window window(std::move(nes));
  window.show();
  return app.exec();
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  if (std::strcmp(argv[1], "--nestest") == 0) {
    if (argc < 3) {
      print_usage(argv[0]);
      return 1;
    }
    return run_nestest(argv[2]);
  }

  if (std::strcmp(argv[1], "--dump-ppm") == 0) {
    if (argc < 5) {
      print_usage(argv[0]);
      return 1;
    }
    return run_dump(std::atoi(argv[2]), argv[3], argv[4]);
  }

  if (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0) {
    print_usage(argv[0]);
    return 0;
  }

  return run_game(argc, argv, argv[1]);
}
