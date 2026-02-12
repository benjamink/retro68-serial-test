# FujiNetCdev - A Retro68 classic Mac Control Panel device for FujiNet configuration
#
# Build instructions:
# mkdir build
# cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# make

cmake_minimum_required(VERSION 3.5)
project(FujiNetCdev)

# Step 1: Build the code resource as a flat binary
add_executable(FujiNetCdev
    src/cdev_main.c
    src/serial.c
    src/dialogs.c
)

set_target_properties(FujiNetCdev PROPERTIES
    OUTPUT_NAME FujiNetCdev.flt
    LINK_FLAGS "-Wl,--mac-flat -Wl,-eCDEV_ENTRY -Wl,-gc-sections"
)

if (REZ_INCLUDE_PATH)
    set(REZ_INCLUDE_FLAG -I${REZ_INCLUDE_PATH})
endif()

# Step 2: Package with Rez into final cdev file
add_custom_command(
    OUTPUT FujiNetCdev.bin FujiNetCdev.dsk
    COMMAND ${REZ} ${REZ_INCLUDE_FLAG}
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/FujiNetCdev.r
                    -o FujiNetCdev.bin
                    --cc FujiNetCdev.dsk
                    --cc FujiNetCdev
                    -t cdev
    DEPENDS FujiNetCdev src/FujiNetCdev.r
)

add_custom_target(FujiNetCdev_cdev ALL DEPENDS FujiNetCdev.dsk)
