#!/bin/bash
# Run FujiNetMacConfig in PCE Mac emulator

set -e

BUILD_ONLY=0
MAKE_DC42=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build-only)
            BUILD_ONLY=1
            shift
            ;;
        -d|--dc42)
            MAKE_DC42=1
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [-b|--build-only] [-d|--dc42]"
            echo "  -b, --build-only  Build and create disk image, but don't start emulator"
            echo "  -d, --dc42        Also create Disk Copy 4.2 image (FujiNetMacConfig.dc42)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

cd "$(dirname "$0")"

# Build if needed
if [ ! -f build/FujiNetMacConfig.bin ]; then
    ./build.sh
fi

# Create a fresh disk image with the app
dd if=/dev/zero of=FujiNetMacConfig.img bs=1024 count=800 2>/dev/null
~/Retro68-build/toolchain/bin/hformat -l "FujiNet Mac Config" FujiNetMacConfig.img >/dev/null
~/Retro68-build/toolchain/bin/hmount FujiNetMacConfig.img >/dev/null
~/Retro68-build/toolchain/bin/hcopy build/FujiNetMacConfig.bin :FujiNetMacConfig >/dev/null
~/Retro68-build/toolchain/bin/humount FujiNetMacConfig.img >/dev/null

# Create Disk Copy 4.2 image if requested
if [ "$MAKE_DC42" -eq 1 ]; then
    python3 "$(dirname "$0")/make_dc42.py" FujiNetMacConfig.img FujiNetMacConfig.dc42 "FujiNet Mac Config"
fi

# Copy disk image to Retro68 build for use with emulator
cp FujiNetMacConfig.img ~/Retro68-build/sample_apps.img

if [ "$BUILD_ONLY" -eq 1 ]; then
    echo "Build complete. Disk image: FujiNetMacConfig.img"
    [ "$MAKE_DC42" -eq 1 ] && echo "Disk Copy 4.2 image: FujiNetMacConfig.dc42"
    exit 0
fi

echo "Launching PCE Mac emulator..."
echo "(The FujiNetMacConfig app is on the 'FujiNet Mac Config' disk)"
echo ""

cd ~/Retro68-build
pkill -9 -f pce-macplus 2>/dev/null || true
pce-macplus -v -c mac-classic.cfg -l pce.log -I rtc.romdisk=0 -r
