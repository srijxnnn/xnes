<div align="center">

![](https://repository-images.githubusercontent.com/1364925191/9f402bce-0b24-4b1a-a4bd-6c31ddc728ef)

# XNES

![code size](https://img.shields.io/github/languages/code-size/srijxnnn/xnes?style=for-the-badge&label=SIZE&color=E60012&labelColor=111111&logo=cplusplus&logoColor=white)![stars](https://img.shields.io/github/stars/srijxnnn/xnes?style=for-the-badge&color=FFD200&labelColor=111111&logo=github&logoColor=white)

</div>

> [!WARNING]
> This repo is work in progess.

XNES is a blazing fast and lightweight NES emulator, written in C++ and uses Qt for the UI. The goal of XNES is to support every possible NES game while being accurate.

We're actively maintaining this repo. Contributions are welcome.

# Status

- [x] 6502 CPU, all 256 opcodes including the unofficial ones
- [x] Cycle counts with page-crossing penalties
- [x] NMI and OAM DMA stalls
- [x] Scanline PPU: background, sprites, sprite 0 hit, overflow flag
- [x] iNES loader, CHR RAM, horizontal/vertical/four-screen mirroring
- [x] NROM (mapper 0)
- [x] Both controller ports
- [x] Instruction trace window
- [ ] APU and sound
- [ ] IRQ sources
- [ ] MMC1, UxROM, CNROM
- [ ] Mid-scanline scroll changes (per-scanline splits already work)
- [ ] NTSC frame pacing, currently a 16 ms Qt timer
- [ ] Save states and battery-backed save RAM
- [ ] PAL

# Installation

You need a C++17 compiler, CMake 3.16 or newer, and Qt 6 Widgets.

```bash
# Arch
sudo pacman -S base-devel cmake qt6-base
# Debian/Ubuntu
sudo apt install build-essential cmake qt6-base-dev
# Fedora
sudo dnf install gcc-c++ cmake qt6-qtbase-devel
```

Clone this repo, then run the following commands in the repo directory.

```bash
cmake -S . -B build
cmake --build build
./build/xnes [rom path]
```

The ROM path is optional. Without it the window opens empty and you can pick a file from **File → Load ROM**. Only iNES (`.nes`) images are accepted.

## Controls

| Key | Action |
| --- | --- |
| <kbd>Z</kbd> | A |
| <kbd>X</kbd> | B |
| <kbd>Shift</kbd> | Select |
| <kbd>Enter</kbd> | Start |
| <kbd>↑</kbd> <kbd>↓</kbd> <kbd>←</kbd> <kbd>→</kbd> | D-pad |
| <kbd>F5</kbd> | Reset |
| <kbd>Esc</kbd> | Quit |

# Contributing

Issues and pull requests are welcome. Open an issue first for anything large, keep changes focused, match the surrounding style, and make sure the build passes before opening a PR.

# License

[GPL-3.0](LICENSE).
