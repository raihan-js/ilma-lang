#!/bin/bash
# Creates the Homebrew tap after a release.
set -e

VERSION=$(cat VERSION)
TARBALL_URL="https://github.com/raihan-js/ilma-lang/archive/refs/tags/v${VERSION}.tar.gz"

echo "Computing SHA256 for v${VERSION}..."
SHA=$(curl -sL "$TARBALL_URL" | shasum -a 256 | cut -d' ' -f1)
echo "SHA256: $SHA"

sed -i "s/FILL_AFTER_RELEASE/$SHA/" Formula/ilma.rb
echo "Updated Formula/ilma.rb with SHA256."

echo ""
echo "Next steps to publish the Homebrew tap:"
echo "1. Create a new GitHub repo named: homebrew-ilma"
echo "   URL: https://github.com/raihan-js/homebrew-ilma"
echo "2. Copy Formula/ilma.rb into that repo as Formula/ilma.rb"
echo "3. Commit and push"
echo ""
echo "Then users can install with:"
echo "  brew tap raihan-js/ilma"
echo "  brew install ilma"
