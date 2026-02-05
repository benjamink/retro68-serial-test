# FujiNetMacConfig - A Retro68 classic Mac application for FujiNet configuration
#
# Build instructions:
# mkdir build
# cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# make

cmake_minimum_required(VERSION 3.5)
project(FujiNetMacConfig)

add_application(FujiNetMacConfig
    src/FujiNetMacConfig.r
    src/main.c
    src/serial.c
    src/window.c
    src/menus.c
    src/dialogs.c
    CREATOR "FNMC"
)

set_target_properties(FujiNetMacConfig PROPERTIES LINK_FLAGS "-Wl,-gc-sections")
