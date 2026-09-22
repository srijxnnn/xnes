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

**CPU**

- [x] Official opcodes
- [x] Unofficial opcodes
- [x] Cycle counts
- [x] Page-crossing penalties
- [x] NMI
- [x] OAM DMA stall
- [ ] IRQ

**PPU**

- [x] Background rendering
- [x] Sprite rendering
- [x] 8x16 sprites
- [x] Sprite 0 hit
- [x] Sprite overflow flag
- [x] Scrolling
- [ ] Mid-scanline scroll changes
- [ ] Greyscale and colour emphasis

**Cartridge**

- [x] iNES header
- [x] CHR RAM
- [x] Horizontal and vertical mirroring
- [x] Four-screen mirroring
- [x] NROM (mapper 0)
- [ ] MMC1
- [ ] UxROM
- [ ] CNROM
- [ ] Battery-backed save RAM

**Input**

- [x] Standard controller
- [x] Both ports on the bus
- [ ] Keys for port 2
- [ ] Remappable keys

**Frontend**

- [x] Load ROM from the menu
- [x] ROM path on the command line
- [x] Integer scaling
- [x] Reset
- [x] CPU trace window
- [ ] Sound
- [ ] Save states
- [ ] Pause and step
- [ ] NTSC frame pacing

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
