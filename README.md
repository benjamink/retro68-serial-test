# FujiNet Mac Config

A classic Macintosh FujiNet configuration application built with [Retro68](https://github.com/autc04/Retro68).

## Features

- Text input field for composing messages
- Configurable serial port (Modem or Printer)
- Configurable baud rate (1200, 2400, 9600, 19200, 38400, 57600)
- Receive area displaying incoming serial data
- FujiNet Reset command via FujiBus protocol
- Standard Mac menus (Apple, File, Edit)
- Keyboard shortcuts: Cmd+S to send, Cmd+Return as alternative
- Host-side Python terminal for bidirectional communication

## Prerequisites

- [Retro68](https://github.com/autc04/Retro68) toolchain built at `~/Retro68-build/`
- CMake 3.5+
- Python 3 with pyserial (for host terminal)
- [tty0tty](https://github.com/freemed/tty0tty) kernel module (for emulator serial communication)
- [PCE Mac Plus](http://www.hampa.ch/pce/pce-macplus.html) emulator

## Building

```bash
./build.sh
```

Or manually:

```bash
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
make
```

### Output Files

| File | Description |
|------|-------------|
| `build/FujiNetMacConfig.bin` | MacBinary format application |
| `build/FujiNetMacConfig.dsk` | 800KB raw HFS disk image |
| `FujiNetMacConfig.dsk` | Copy of above (for emulator) |

## Running in Emulator

```bash
./run.sh
```

This creates a bootable disk image, copies the app to `~/Retro68-build/sample_apps.img`, and launches the PCE Mac Plus emulator.

## Serial Communication

### Emulator Setup

The app uses **Port A (Modem)** by default, which connects to `/dev/tnt1` in the emulator. The host connects to `/dev/tnt0` via the tty0tty null-modem driver.

```
┌─────────────────┐     tty0tty      ┌──────────────────┐
│  PCE Emulator   │                  │   Host Machine   │
│  (Mac Classic)  │                  │                  │
│                 │                  │                  │
│ FujiNetMacConfig│◄────/dev/tnt1────►│  serial_terminal │
│     Port A      │     /dev/tnt0    │                  │
└─────────────────┘                  └──────────────────┘
```

### Loading tty0tty

```bash
sudo modprobe tty0tty
sudo chmod 666 /dev/tnt*
```

### Host Terminal

Interactive bidirectional terminal:

```bash
./serial_terminal.py                  # Interactive mode
./serial_terminal.py --bot            # Bot mode (responds to @bot commands)
./serial_terminal.py -s "Hello Mac!"  # Send text
./serial_terminal.py -f script.txt    # Send file contents
./serial_terminal.py -b 19200         # Different baud rate
```

Bot mode commands:
- `@bot hello` - Get a greeting
- `@bot time` - Current time
- `@bot date` - Current date
- `@bot ping` - Pong!
- `@bot echo <text>` - Echo text back

### Port B (Printer)

Port B outputs to a file instead of a virtual serial device:

```bash
tail -f ~/Retro68-build/ser_b.out
# or
./serial_terminal.py -w
```

## Settings

Access via **File > Settings** (Cmd+,) to configure:

- **Port**: Modem (Port A) or Printer (Port B)
- **Baud Rate**: 1200, 2400, 9600, 19200, 38400, 57600

Default: Modem port at 9600 baud, 8N1, no flow control.

## Project Structure

```
├── src/
│   ├── main.c              # Application entry point and event loop
│   ├── serial.c            # Serial port and FujiBus protocol
│   ├── window.c            # Window creation and drawing
│   ├── menus.c             # Menu handling
│   ├── dialogs.c           # Settings dialog
│   └── FujiNetMacConfig.r  # Rez resource file (menus, dialogs, icons)
├── CMakeLists.txt          # Build configuration
├── build.sh                # Build script
├── run.sh                  # Emulator launch script
└── serial_terminal.py      # Host-side serial terminal
```

### Source Functions

| Function | Description |
|----------|-------------|
| `InitializeToolbox()` | Standard Mac Toolbox initialization |
| `InitializeSerial()` | Opens and configures serial port |
| `CreateMainWindow()` | Creates window with TextEdit fields and button |
| `HandleEvent()` | Main event dispatch loop |
| `SendTextToSerial()` | Sends text with CR→CRLF conversion |
| `SendResetCommand()` | Sends FujiBus Reset to FujiNet |
| `PollSerialInput()` | Reads incoming serial data |
| `DoSettingsDialog()` | Port and baud rate configuration |

## Emulator Configuration

PCE Mac Plus config at `~/Retro68-build/mac-classic.cfg`:

```
serial {
    port = 0
    driver = "posix"
    file = "/dev/tnt1"
}

serial {
    port = 1
    driver = "stdio"
    file = "ser_b.out"
}
```

## License

This project is provided as-is for educational purposes.
