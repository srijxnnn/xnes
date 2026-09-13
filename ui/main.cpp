#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "logger.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    return 1;
  }

  auto cart = Cartridge::load(argv[1]);
  if (!cart) {
    return 1;
  }

  Bus bus;
  bus.insert(cart.value());

  CPU cpu(bus);
  cpu.reset();
  cpu.set_pc(0xC000);

  Logger logger;

  while (true) {
    logger.log(cpu);

    cpu.step();

    if (cpu.halt) {
      break;
    }
  }
}