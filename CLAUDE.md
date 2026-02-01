# SerialSend - Retro68 Classic Mac Application

## Project Overview
A classic Macintosh application built with Retro68 that displays a window with a text input field and a "Send" button. Text entered is sent to the serial port.

## Build System

### Prerequisites
- Retro68 toolchain built at `~/Retro68-build/`
- CMake 3.5+

### Building
```bash
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
make
```

Or use the convenience script:
```bash
./build.sh
```

### Output Files
- `build/SerialSend.bin` - MacBinary format application
- `build/SerialSend.dsk` - 800KB raw HFS disk image
- `SerialSend.dsk` - Copy of above in project root (for emulator)
- `SerialSend.img` - Disk Copy 4.2 format (for real Macs)

### Disk Copy 4.2 Conversion
The `make_dc42.py` script converts raw disk images to Disk Copy 4.2 format:
```bash
python3 make_dc42.py input.dsk output.dc42 "Disk Name"
```

### Running in Emulator
```bash
./run.sh
```
This copies the app to `~/Retro68-build/sample_apps.img` and launches PCE Mac emulator.

## Serial Port Configuration

### Current Setup
- Uses **Port A** (modem port) - `.AOut` / `.AIn` drivers
- Configured for 9600 baud, 8N1, no flow control
- Connects to `/dev/tnt1` in emulator (use `/dev/tnt0` on host via tty0tty)

### Serial Terminal Tool
Interactive terminal for bidirectional serial communication:
```bash
./serial_terminal.py           # Interactive terminal on /dev/tnt0
./serial_terminal.py -s "Hi"   # Send text
./serial_terminal.py -f file   # Send file contents
./serial_terminal.py -w        # Watch ser_b.out (port B output)
```
Requires: `uv pip install pyserial` and tty0tty kernel module.

### Monitoring Port B Output
```bash
tail -f ~/Retro68-build/ser_b.out
```

### Switching to Port B (Printer)
Change `.AOut`/`.AIn` to `.BOut`/`.BIn` in `InitializeSerial()`. Port B outputs to `~/Retro68-build/ser_b.out` file (output only, no input).

## Code Structure

### main.c
- `InitializeToolbox()` - Standard Mac Toolbox initialization
- `InitializeSerial()` - Opens serial port, configures baud rate
- `CreateMainWindow()` - Creates window with TextEdit field and button
- `HandleEvent()` - Main event dispatch
- `SendTextToSerial()` - Sends text with CR→CRLF conversion

### SerialSend.r
Rez resource file containing:
- Menu bar (Apple, File, Edit menus)
- SIZE resource for MultiFinder
- About dialog (DLOG/DITL 128)
- Application icons (ICN#, ics#)
- Finder bundle (BNDL, FREF)

## Classic Mac Toolbox Notes

### Include Headers
Use the universal headers from Retro68:
- `<Quickdraw.h>`, `<Windows.h>`, `<TextEdit.h>`, etc.
- `<ControlDefinitions.h>` for `pushButProc`, `kControlButtonPart`
- `<Sound.h>` for `SysBeep()`
- Use new-style constants: `kFontIDMonaco` not `monaco`, `kControlButtonPart` not `inButton`

### TextEdit
- `TENew(destRect, viewRect)` - destRect controls text wrapping, viewRect controls clipping
- For a framed text area, inset both rects from the frame to prevent drawing over the border
- Mac uses CR (`\r`) for line endings internally

### Serial I/O
- `OpenDriver("\p.AOut", &refNum)` - Modem port output
- `OpenDriver("\p.BOut", &refNum)` - Printer port output
- `FSWrite(refNum, &count, buffer)` - Write to serial
- `SerReset(refNum, baud9600 + stop10 + noParity + data8)` - Configure

### Pascal Strings
Use `\p` prefix for Pascal strings: `"\pHello"` creates a length-prefixed string.

## Emulator Configuration
PCE Mac Classic config at `~/Retro68-build/mac-classic.cfg`:
- Port 0 (modem): `posix:file=/dev/tnt1`
- Port 1 (printer): `stdio:file=ser_b.out`
- Disk 1: `sample_apps.img` (where app is copied)
