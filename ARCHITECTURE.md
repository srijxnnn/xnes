# Architecture

This is the architecture of XNES.

# Repository

The repository is split in 2 parts: UI and emulator core.

The emulator core is decoupled from UI code. Emulator core is not aware of any code in UI. However, UI should be able to call any core functions.

The emulator core currently consists of 3 parts: Bus, Cartridge, CPU.

# CPU

