# SerialSend - A simple Retro68 application that sends text to serial port
#
# Build instructions:
# mkdir build
# cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# make

cmake_minimum_required(VERSION 3.5)
project(SerialSend)

add_application(SerialSend
    SerialSend.r
    main.c
    serial.c
    window.c
    menus.c
    dialogs.c
    CREATOR "SSND"
)

set_target_properties(SerialSend PROPERTIES LINK_FLAGS "-Wl,-gc-sections")
