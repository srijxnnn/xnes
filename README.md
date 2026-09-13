<p align="center">
  <img src="https://repository-images.githubusercontent.com/1364925191/9f402bce-0b24-4b1a-a4bd-6c31ddc728ef">
</p>

<h1 align="center">
XNES
</h1>

<p align="center">
  <img src="https://img.shields.io/github/languages/code-size/srijxnnn/xnes?style=for-the-badge&label=SIZE&color=E60012&labelColor=111111&logo=cplusplus&logoColor=white" alt="code size">
  <img src="https://img.shields.io/github/stars/srijxnnn/xnes?style=for-the-badge&color=FFD200&labelColor=111111&logo=github&logoColor=white" alt="stars">
</p>


XNES is a blazing fast and lightweight NES emulator, written in C++ and uses Qt for the UI. The goal of XNES is to support every possible NES game while being accurate.

We're actively maintaining this repo. Contributions are welcome.

# AI Usage
You cannot use AI to write code ("vibecode") for you. Code must be written and understood by you. However, you can definitely use AI to learn quicker.

# Architecture
Read [ARCHITECTURE.md](ARCHITECTURE.md).

# Installation
Clone this repo, then run the following commands in the repo directory.

```bash
cmake -S . -B build
cmake --build build
./build/xnes [rom path]
```