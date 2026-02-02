#!/usr/bin/env python3
"""
Convert a raw HFS disk image to Disk Copy 4.2 format.

Disk Copy 4.2 format:
- 84-byte header
- Data section (raw disk data)
- Tag section (optional, usually empty for 800K)

Usage: make_dc42.py input.img output.dc42 "Disk Name"
"""

import struct
import sys


def calc_checksum(data: bytes) -> int:
    """Calculate DC42 checksum (rotating XOR)."""
    checksum = 0
    for i in range(0, len(data), 2):
        if i + 1 < len(data):
            word = (data[i] << 8) | data[i + 1]
        else:
            word = data[i] << 8
        checksum = ((checksum + word) & 0xFFFFFFFF)
        checksum = ((checksum >> 1) | (checksum << 31)) & 0xFFFFFFFF
    return checksum


def make_dc42(input_path: str, output_path: str, disk_name: str) -> None:
    """Convert raw disk image to Disk Copy 4.2 format."""
    with open(input_path, "rb") as f:
        data = f.read()

    data_size = len(data)

    # Determine disk format based on size
    if data_size == 400 * 1024:
        disk_format = 0x00  # 400K GCR
        format_byte = 0x12
    elif data_size == 800 * 1024:
        disk_format = 0x01  # 800K GCR
        format_byte = 0x22
    elif data_size == 720 * 1024:
        disk_format = 0x02  # 720K MFM
        format_byte = 0x02
    elif data_size == 1440 * 1024:
        disk_format = 0x03  # 1440K MFM
        format_byte = 0x24
    else:
        # Assume 800K format for other sizes (will pad or truncate)
        print(f"Warning: unusual disk size {data_size}, assuming 800K format")
        disk_format = 0x01
        format_byte = 0x22

    # No tag data for simplicity
    tag_size = 0
    tag_data = b""
    tag_checksum = 0

    # Calculate data checksum
    data_checksum = calc_checksum(data)

    # Build the 84-byte header
    # Pascal string for disk name (1 byte length + 63 bytes name)
    name_bytes = disk_name.encode("mac_roman", errors="replace")[:63]
    name_pascal = bytes([len(name_bytes)]) + name_bytes.ljust(63, b"\x00")

    header = bytearray(84)
    header[0:64] = name_pascal
    struct.pack_into(">I", header, 64, data_size)      # data size
    struct.pack_into(">I", header, 68, tag_size)       # tag size
    struct.pack_into(">I", header, 72, data_checksum)  # data checksum
    struct.pack_into(">I", header, 76, tag_checksum)   # tag checksum
    header[80] = disk_format                            # disk format
    header[81] = format_byte                            # format byte
    struct.pack_into(">H", header, 82, 0x0100)         # private

    # Write output file
    with open(output_path, "wb") as f:
        f.write(header)
        f.write(data)
        if tag_data:
            f.write(tag_data)

    print(f"Created {output_path} ({data_size} bytes, format {disk_format:#x})")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} input.img output.dc42 'Disk Name'")
        sys.exit(1)

    make_dc42(sys.argv[1], sys.argv[2], sys.argv[3])
