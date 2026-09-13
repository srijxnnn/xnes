cmake_minimum_required(VERSION 3.16)

project(xnes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(xnes_core core/cartridge.cpp core/bus.cpp core/cpu.cpp)
target_include_directories(xnes_core PUBLIC core)

add_executable(xnes ui/main.cpp ui/logger.cpp)
target_link_libraries(xnes PRIVATE xnes_core Qt6::Widgets)