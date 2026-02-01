#!/usr/bin/env python3
"""Convert raw disk image to Disk Copy 4.2 format"""
import sys
import struct

def dc42_checksum(data):
    """Calculate DC42 checksum - rotating checksum over 16-bit words"""
    cksum = 0
    # Process data in 16-bit big-endian words
    for i in range(0, len(data) - 1, 2):
        word = (data[i] << 8) | data[i + 1]
        cksum += word
        cksum = ((cksum >> 1) | ((cksum & 1) << 31)) & 0xFFFFFFFF
    # Handle odd byte at end if present
    if len(data) % 2 == 1:
        word = data[-1] << 8
        cksum += word
        cksum = ((cksum >> 1) | ((cksum & 1) << 31)) & 0xFFFFFFFF
    return cksum

def make_dc42(input_path, output_path, disk_name="Untitled"):
    with open(input_path, 'rb') as f:
        data = f.read()

    data_size = len(data)

    # Determine disk type based on size
    if data_size == 400 * 1024:
        encoding = 0x00  # GCR 400K
        format_byte = 0x12  # Single-sided
    elif data_size == 800 * 1024:
        encoding = 0x01  # GCR 800K
        format_byte = 0x22  # Double-sided
    elif data_size == 720 * 1024:
        encoding = 0x02  # MFM 720K
        format_byte = 0x22
    elif data_size == 1440 * 1024:
        encoding = 0x03  # MFM 1440K
        format_byte = 0x24  # High-density
    else:
        print(f"Error: Non-standard disk size {data_size} bytes", file=sys.stderr)
        print("Supported sizes: 400K, 720K, 800K, 1440K", file=sys.stderr)
        sys.exit(1)

    # Build 84-byte header
    header = bytearray(84)

    # Disk name as Pascal string (length byte + up to 63 chars)
    # Pad with zeros
    name_bytes = disk_name.encode('latin-1')[:63]
    header[0] = len(name_bytes)
    header[1:1+len(name_bytes)] = name_bytes
    # Rest of name field (up to byte 63) stays zero

    # Data fork size at offset 64 (big endian)
    struct.pack_into('>I', header, 64, data_size)

    # Tag data size at offset 68 (0 for 3.5" disks)
    struct.pack_into('>I', header, 68, 0)

    # Data checksum at offset 72
    data_cksum = dc42_checksum(data)
    struct.pack_into('>I', header, 72, data_cksum)

    # Tag checksum at offset 76 (0 since no tag data)
    struct.pack_into('>I', header, 76, 0)

    # Disk encoding at offset 80
    header[80] = encoding

    # Format byte at offset 81
    header[81] = format_byte

    # Magic number at offset 82-83 (0x0100)
    header[82] = 0x01
    header[83] = 0x00

    with open(output_path, 'wb') as f:
        f.write(header)
        f.write(data)

    print(f"Created {output_path}")
    print(f"  Size: {data_size} bytes ({data_size // 1024}K)")
    print(f"  Checksum: 0x{data_cksum:08X}")
    print(f"  Encoding: 0x{encoding:02X}, Format: 0x{format_byte:02X}")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} input.dsk output.img [disk_name]")
        sys.exit(1)

    disk_name = sys.argv[3] if len(sys.argv) > 3 else "SerialSend"
    make_dc42(sys.argv[1], sys.argv[2], disk_name)
