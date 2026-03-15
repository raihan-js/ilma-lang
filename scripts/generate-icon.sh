#!/bin/bash
# Generate the ILMA VS Code extension icon
# Requires: ImageMagick (convert command)
set -e

OUTDIR="$(cd "$(dirname "$0")/../vscode-extension/images" && pwd)"

if ! command -v convert &>/dev/null; then
    echo "Error: ImageMagick (convert) not found."
    echo "Install: sudo apt install imagemagick"
    exit 1
fi

convert -size 128x128 xc:'#534AB7' \
    -fill white -font DejaVu-Sans-Bold \
    -pointsize 52 -gravity Center -annotate 0 "IL" \
    "$OUTDIR/ilma-icon.png"

echo "Icon generated: $OUTDIR/ilma-icon.png"
