#!/bin/bash
# Run SerialSend in PCE Mac emulator

set -e

cd "$(dirname "$0")"

# Build if needed
if [ ! -f build/SerialSend.bin ]; then
    ./build.sh
fi

# Create a fresh disk image with the app
dd if=/dev/zero of=SerialSend.img bs=1024 count=8000 2>/dev/null
~/Retro68-build/toolchain/bin/hformat -l "Serial Send" SerialSend.img >/dev/null
~/Retro68-build/toolchain/bin/hmount SerialSend.img >/dev/null
~/Retro68-build/toolchain/bin/hcopy build/SerialSend.bin :SerialSend >/dev/null
~/Retro68-build/toolchain/bin/humount SerialSend.img >/dev/null

# Copy disk image to Retro68 build for use with emulator
cp SerialSend.img ~/Retro68-build/sample_apps.img

echo "Launching PCE Mac emulator..."
echo "(The SerialSend app is on the 'Serial Send' disk)"
echo ""

cd ~/Retro68-build
pkill -9 -f pce-macplus 2>/dev/null || true
pce-macplus -v -c mac-classic.cfg -l pce.log -I rtc.romdisk=0 -r
