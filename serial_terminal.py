#!/usr/bin/env python3
"""
Interactive serial terminal for PCE Mac emulator.
Connects to /dev/tnt0 (pairs with emulator's /dev/tnt1 via tty0tty).
"""

import sys
import os
import select
import termios
import tty
import argparse
import time


def open_serial(device, baud=9600):
    """Open serial port with specified baud rate."""
    import serial
    return serial.Serial(device, baud, timeout=0)


def handle_bot_message(message):
    """Generate a response to a @bot message."""
    # Strip the @bot prefix and whitespace
    content = message.strip()
    if content.lower().startswith('@bot'):
        content = content[4:].strip()

    # Simple responses based on content
    content_lower = content.lower()

    if not content:
        return "Hello! I'm a bot. Send me a message after @bot."
    elif 'hello' in content_lower or 'hi' in content_lower:
        return "Hello from the host machine!"
    elif 'time' in content_lower:
        return f"Current time: {time.strftime('%H:%M:%S')}"
    elif 'date' in content_lower:
        return f"Today is {time.strftime('%Y-%m-%d')}"
    elif 'ping' in content_lower:
        return "Pong!"
    elif 'help' in content_lower:
        return "Commands: hello, time, date, ping, echo <text>"
    elif content_lower.startswith('echo '):
        return content[5:]
    else:
        return f"Received: {content}"


def run_terminal(device, baud, bot_mode=False):
    """Run interactive terminal."""
    try:
        ser = open_serial(device, baud)
    except Exception as e:
        print(f"Error opening {device}: {e}")
        print("\nMake sure tty0tty module is loaded:")
        print("  sudo modprobe tty0tty")
        print("  sudo chmod 666 /dev/tnt*")
        return 1

    print(f"Connected to {device} at {baud} baud")
    if bot_mode:
        print("Bot mode enabled - will respond to @bot messages")
    print("Press Ctrl+C to exit, Ctrl+L to clear screen")
    print("-" * 40)

    # Save terminal settings
    old_settings = termios.tcgetattr(sys.stdin)

    # Buffer for incoming lines (for bot detection)
    line_buffer = ""

    try:
        # Set terminal to raw mode
        tty.setraw(sys.stdin.fileno())

        while True:
            # Check for input from keyboard or serial
            readable, _, _ = select.select([sys.stdin, ser], [], [], 0.1)

            for source in readable:
                if source == sys.stdin:
                    # Read from keyboard
                    char = sys.stdin.read(1)

                    if char == '\x03':  # Ctrl+C
                        raise KeyboardInterrupt
                    elif char == '\x0c':  # Ctrl+L - clear screen
                        sys.stdout.write('\033[2J\033[H')
                        sys.stdout.flush()
                        continue
                    elif char == '\r':  # Enter key
                        # Send CR+LF to serial
                        ser.write(b'\r\n')
                        # Echo newline locally
                        sys.stdout.write('\r\n')
                        sys.stdout.flush()
                    else:
                        # Send character to serial
                        ser.write(char.encode('latin-1'))
                        # Echo locally
                        sys.stdout.write(char)
                        sys.stdout.flush()

                elif source == ser:
                    # Read from serial
                    data = ser.read(256)
                    if data:
                        text = data.decode('latin-1', errors='replace')

                        # Display received text
                        display_text = text.replace('\r\n', '\r\n')
                        display_text = display_text.replace('\r', '\r\n')
                        sys.stdout.write(display_text)
                        sys.stdout.flush()

                        # Bot mode: buffer and check for @bot messages
                        if bot_mode:
                            line_buffer += text
                            # Process complete lines
                            while '\r' in line_buffer or '\n' in line_buffer:
                                # Find end of line
                                idx = -1
                                for i, c in enumerate(line_buffer):
                                    if c in '\r\n':
                                        idx = i
                                        break
                                if idx == -1:
                                    break

                                line = line_buffer[:idx]
                                # Skip past line ending (CR, LF, or CRLF)
                                line_buffer = line_buffer[idx+1:]
                                if line_buffer.startswith('\n'):
                                    line_buffer = line_buffer[1:]

                                # Check for @bot command
                                if line.strip().lower().startswith('@bot'):
                                    response = handle_bot_message(line)
                                    # Send response after small delay
                                    time.sleep(0.1)
                                    response_bytes = (response + '\r\n')
                                    ser.write(response_bytes.encode('latin-1'))
                                    # Echo response locally
                                    sys.stdout.write(f"\r\n[BOT] {response}\r\n")
                                    sys.stdout.flush()

    except KeyboardInterrupt:
        pass
    finally:
        # Restore terminal settings
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
        ser.close()
        print("\r\nDisconnected.")

    return 0


def send_file(device, baud, filename):
    """Send a file to the serial port."""
    try:
        ser = open_serial(device, baud)
    except Exception as e:
        print(f"Error opening {device}: {e}")
        return 1

    try:
        with open(filename, 'r') as f:
            content = f.read()

        # Convert newlines to CR+LF
        content = content.replace('\r\n', '\n')
        content = content.replace('\r', '\n')
        content = content.replace('\n', '\r\n')

        ser.write(content.encode('latin-1'))
        print(f"Sent {len(content)} bytes from {filename}")

    except FileNotFoundError:
        print(f"File not found: {filename}")
        return 1
    finally:
        ser.close()

    return 0


def send_text(device, baud, text):
    """Send text to the serial port."""
    try:
        ser = open_serial(device, baud)
    except Exception as e:
        print(f"Error opening {device}: {e}")
        return 1

    # Add CR+LF if not present
    if not text.endswith('\n') and not text.endswith('\r'):
        text += '\r\n'
    else:
        text = text.replace('\n', '\r\n')

    ser.write(text.encode('latin-1'))
    print(f"Sent: {repr(text)}")
    ser.close()
    return 0


def watch_file(filepath):
    """Watch ser_b.out file for new output (for port B)."""
    print(f"Watching {filepath} (Ctrl+C to stop)")
    print("-" * 40)

    try:
        with open(filepath, 'rb') as f:
            # Seek to end
            f.seek(0, 2)

            while True:
                line = f.read(256)
                if line:
                    text = line.decode('latin-1', errors='replace')
                    text = text.replace('\r\n', '\n').replace('\r', '\n')
                    sys.stdout.write(text)
                    sys.stdout.flush()
                else:
                    time.sleep(0.1)
    except KeyboardInterrupt:
        print("\nStopped.")
    except FileNotFoundError:
        print(f"File not found: {filepath}")
        print("Start the emulator first, or check the path.")
        return 1

    return 0


def main():
    parser = argparse.ArgumentParser(
        description='Serial terminal for PCE Mac emulator',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                    Interactive terminal on /dev/tnt0
  %(prog)s --bot              Enable bot mode (respond to @bot messages)
  %(prog)s -d /dev/ttyUSB0    Use different serial device
  %(prog)s -s "Hello World"   Send text and exit
  %(prog)s -f script.txt      Send file contents
  %(prog)s -w                 Watch ser_b.out file (port B output)

Bot commands (when --bot enabled):
  @bot hello                  Get a greeting
  @bot time                   Get current time
  @bot date                   Get current date
  @bot ping                   Pong!
  @bot echo <text>            Echo text back
  @bot help                   List commands
""")

    parser.add_argument('-d', '--device', default='/dev/tnt0',
                        help='Serial device (default: /dev/tnt0)')
    parser.add_argument('-b', '--baud', type=int, default=9600,
                        help='Baud rate (default: 9600)')
    parser.add_argument('-s', '--send', metavar='TEXT',
                        help='Send text and exit')
    parser.add_argument('-f', '--file', metavar='FILE',
                        help='Send file contents and exit')
    parser.add_argument('-w', '--watch', action='store_true',
                        help='Watch ser_b.out file instead of using tty')
    parser.add_argument('--bot', action='store_true',
                        help='Enable bot mode - respond to @bot messages')
    parser.add_argument(
        '--watch-file',
        default=os.path.expanduser('~/Retro68-build/ser_b.out'),
        help='File to watch with -w (default: ~/Retro68-build/ser_b.out)')

    args = parser.parse_args()

    # Check for pyserial
    if not args.watch:
        try:
            import serial  # noqa: F401
        except ImportError:
            print("Error: pyserial not installed")
            print("Install with: uv pip install pyserial")
            return 1

    if args.watch:
        return watch_file(args.watch_file)
    elif args.send:
        return send_text(args.device, args.baud, args.send)
    elif args.file:
        return send_file(args.device, args.baud, args.file)
    else:
        return run_terminal(args.device, args.baud, bot_mode=args.bot)


if __name__ == '__main__':
    sys.exit(main())
