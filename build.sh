#!/bin/bash
# Build script for Cloudflare Pages
# Copies all assets into website/ before deployment
set -e

echo "==> Copying install scripts..."
cp install.sh website/install.sh
chmod +x website/install.sh
sed -i 's/\r//' website/install.sh
cp scripts/install-windows.ps1 website/install.ps1

echo "==> Writing version file..."
cp VERSION website/VERSION
VERSION=$(cat VERSION | tr -d '[:space:]')
cat > website/ilma-version.json << EOF
{
  "version": "${VERSION}",
  "repo": "https://github.com/raihan-js/ilma-lang",
  "install": "curl -fsSL https://ilma-lang.dev/install.sh | bash",
  "website": "https://ilma-lang.dev"
}
EOF

echo "==> Copying packages registry..."
mkdir -p website/packages
cp -r packages/* website/packages/
python3 scripts/build-registry.py
cp packages/registry.json website/packages/registry.json

echo "==> Build complete. Output: website/"
