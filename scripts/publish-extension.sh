#!/bin/bash
# Publish the ILMA VS Code Extension to the Marketplace
# Requirements: npm install -g @vscode/vsce, valid publisher account
set -e

cd "$(dirname "$0")/../vscode-extension"

echo "Installing dependencies..."
npm install

echo "Compiling extension..."
npm run compile

echo "Packaging..."
npx vsce package

echo "Publishing..."
npx vsce publish

echo ""
echo "Done! Check: https://marketplace.visualstudio.com/items?itemName=ilmalang.ilma-language"
