cmake_minimum_required(VERSION 3.16)

project(xnes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(xnes_core
  core/bus/cpu_bus.cpp
  core/bus/ppu_bus.cpp
  core/cartridge/cartridge.cpp
  core/controller/controller.cpp
  core/cpu/cpu.cpp
  core/cpu/cpu_table.cpp
  core/mapper/mapper.cpp
  core/mapper/nrom.cpp
  core/nes/nes.cpp
  core/ppu/ppu.cpp
)
target_include_directories(xnes_core PUBLIC core)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(xnes ui/main.cpp ui/window.cpp)
target_include_directories(xnes PRIVATE ui)
target_link_libraries(xnes PRIVATE xnes_core Qt6::Widgets)

enable_testing()
add_test(
  NAME nestest
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test/run_nestest.sh
          $<TARGET_FILE:xnes>
          ${CMAKE_CURRENT_SOURCE_DIR}/test/nestest.nes
          ${CMAKE_CURRENT_SOURCE_DIR}/test/nestest.log
)
