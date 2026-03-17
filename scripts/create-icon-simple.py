#!/usr/bin/env python3
"""
Generates a minimal 128x128 PNG icon for the ILMA VS Code extension.
Requires the 'Pillow' library (pip install Pillow).
If Pillow is not available, generates a minimal valid PNG from scratch.
"""

import os
import struct
import zlib

def create_minimal_png(path, size=128):
    """Create a minimal green square PNG without any dependencies."""
    width = height = size

    # PNG signature
    signature = b'\x89PNG\r\n\x1a\n'

    # IHDR chunk: width, height, bit_depth=8, color_type=2 (RGB)
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)
    ihdr = make_chunk(b'IHDR', ihdr_data)

    # IDAT chunk: image data
    # Color: #2d6a4f (45, 106, 79) — ILMA green
    r, g, b = 45, 106, 79
    raw_rows = []
    for y in range(height):
        row = b'\x00' + bytes([r, g, b] * width)
        raw_rows.append(row)
    raw_data = b''.join(raw_rows)
    compressed = zlib.compress(raw_data, 9)
    idat = make_chunk(b'IDAT', compressed)

    # IEND chunk
    iend = make_chunk(b'IEND', b'')

    png_data = signature + ihdr + idat + iend

    with open(path, 'wb') as f:
        f.write(png_data)

    print(f'Created icon: {path} ({len(png_data)} bytes)')


def make_chunk(chunk_type, data):
    """Create a PNG chunk with CRC."""
    length = struct.pack('>I', len(data))
    crc = struct.pack('>I', zlib.crc32(chunk_type + data) & 0xffffffff)
    return length + chunk_type + data + crc


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)
    images_dir = os.path.join(project_dir, 'vscode-extension', 'images')
    os.makedirs(images_dir, exist_ok=True)
    icon_path = os.path.join(images_dir, 'ilma-icon.png')

    # Try Pillow first for a nicer icon
    try:
        from PIL import Image, ImageDraw, ImageFont
        img = Image.new('RGB', (128, 128), color=(45, 106, 79))
        draw = ImageDraw.Draw(img)
        # Draw a simple "I" letter
        draw.rectangle([54, 30, 74, 98], fill=(255, 255, 255))
        draw.rectangle([34, 30, 94, 46], fill=(255, 255, 255))
        draw.rectangle([34, 82, 94, 98], fill=(255, 255, 255))
        img.save(icon_path, 'PNG')
        print(f'Created icon with Pillow: {icon_path}')
    except ImportError:
        create_minimal_png(icon_path)


if __name__ == '__main__':
    main()
