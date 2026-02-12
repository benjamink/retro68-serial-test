#!/bin/bash
# Build script for FujiNetCdev

set -e

cd "$(dirname "$0")"

# Create build directory if needed
mkdir -p build
cd build

# Configure with cmake
cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake

# Build
make

cd ..

# Copy disk image to project root
cp build/FujiNetCdev.dsk .

# Convert to Disk Copy 4.2 format for real Macs
python3 make_dc42.py build/FujiNetCdev.dsk FujiNetCdev.img "FujiNet Cdev"

echo ""
echo "Build complete!"
echo "Disk images:"
echo "  FujiNetCdev.dsk - Raw HFS (for emulator)"
echo "  FujiNetCdev.img - Disk Copy 4.2 (for real Mac)"
