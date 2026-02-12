#!/usr/bin/env python3
"""Convert logo PNG to 1-bit bitmap C data for classic Mac Toolbox CopyBits."""

from PIL import Image
import sys

def convert_logo(input_path, target_width=128):
    img = Image.open(input_path).convert("L")  # grayscale

    # Crop to content bounding box (remove transparent/black borders)
    pixels_orig = img.load()
    min_x, min_y, max_x, max_y = img.width, img.height, 0, 0
    for y in range(img.height):
        for x in range(img.width):
            if pixels_orig[x, y] > 100:
                min_x = min(min_x, x)
                min_y = min(min_y, y)
                max_x = max(max_x, x)
                max_y = max(max_y, y)

    if max_x <= min_x or max_y <= min_y:
        print("Error: no content found in image", file=sys.stderr)
        return

    # Add small padding
    pad = 2
    min_x = max(0, min_x - pad)
    min_y = max(0, min_y - pad)
    max_x = min(img.width, max_x + pad + 1)
    max_y = min(img.height, max_y + pad + 1)

    img = img.crop((min_x, min_y, max_x, max_y))
    print(f"/* Cropped from {min_x},{min_y} to {max_x},{max_y} */", file=sys.stderr)

    # Scale to target width, maintaining aspect ratio
    aspect = img.height / img.width
    target_height = int(target_width * aspect)
    img = img.resize((target_width, target_height), Image.LANCZOS)

    pixels = img.load()

    # rowBytes must be even for Mac BitMap
    row_bytes = (target_width + 15) // 16 * 2

    print(f"/* Logo bitmap: {target_width}x{target_height}, rowBytes={row_bytes} */")
    print(f"#define kLogoBitmapWidth  {target_width}")
    print(f"#define kLogoBitmapHeight {target_height}")
    print(f"#define kLogoRowBytes     {row_bytes}")
    print(f"")
    print(f"static const unsigned char gLogoData[] = {{")

    for y in range(target_height):
        row_bytes_data = []
        for byte_idx in range(row_bytes):
            byte_val = 0
            for bit in range(8):
                x = byte_idx * 8 + bit
                if x < target_width:
                    pixel = pixels[x, y]
                    if pixel > 80:  # threshold
                        byte_val |= (0x80 >> bit)
            row_bytes_data.append(byte_val)

        hex_str = ", ".join(f"0x{b:02X}" for b in row_bytes_data)
        comma = "," if y < target_height - 1 else ""
        print(f"    {hex_str}{comma}")

    print(f"}};")

if __name__ == "__main__":
    input_path = sys.argv[1] if len(sys.argv) > 1 else "fujinet-logo-white-transparent.png"
    target_width = int(sys.argv[2]) if len(sys.argv) > 2 else 128
    convert_logo(input_path, target_width)
