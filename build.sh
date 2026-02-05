#!/bin/bash
# Build script for FujiNetMacConfig

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
cp build/FujiNetMacConfig.dsk .

# Convert to Disk Copy 4.2 format for real Macs
python3 make_dc42.py build/FujiNetMacConfig.dsk FujiNetMacConfig.img "FujiNet Mac Config"

echo ""
echo "Build complete!"
echo "Disk images:"
echo "  FujiNetMacConfig.dsk - Raw HFS (for emulator)"
echo "  FujiNetMacConfig.img - Disk Copy 4.2 (for real Mac)"
