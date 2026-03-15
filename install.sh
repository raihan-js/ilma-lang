#!/usr/bin/env bash
# ILMA Programming Language Installer
# Usage: curl -fsSL https://ilma-lang.dev/install.sh | bash
# Or:    bash install.sh

set -e

ILMA_VERSION="0.1.0"
ILMA_REPO="https://github.com/raihan-js/ilma-lang"
INSTALL_DIR="/usr/local"
BIN_DIR="$INSTALL_DIR/bin"
LIB_DIR="$INSTALL_DIR/lib/ilma/runtime"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m'

print_banner() {
    echo ""
    echo -e "${BOLD}${BLUE}ILMA Programming Language${NC}"
    echo -e "${BLUE}ilma-lang.dev${NC}"
    echo -e "Version ${ILMA_VERSION}"
    echo ""
}

check_dependency() {
    if ! command -v "$1" &>/dev/null; then
        echo -e "${RED}Error: '$1' is required but not installed.${NC}"
        echo "Install it with:"
        case "$1" in
            gcc)   echo "  Ubuntu/Debian: sudo apt install gcc" ;;
            git)   echo "  Ubuntu/Debian: sudo apt install git" ;;
            make)  echo "  Ubuntu/Debian: sudo apt install make" ;;
        esac
        exit 1
    fi
}

detect_os() {
    OS="unknown"
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        OS="windows"
    fi
    echo $OS
}

install_from_source() {
    local tmpdir
    tmpdir=$(mktemp -d)
    echo -e "${BLUE}Cloning ILMA repository...${NC}"
    git clone --depth=1 "$ILMA_REPO.git" "$tmpdir/ilma" 2>/dev/null || {
        echo -e "${RED}Failed to clone repository. Check your internet connection.${NC}"
        exit 1
    }
    cd "$tmpdir/ilma"
    echo -e "${BLUE}Building ILMA compiler...${NC}"
    make all
    echo -e "${BLUE}Installing to $INSTALL_DIR...${NC}"
    if [[ "$EUID" -ne 0 ]]; then
        sudo make install PREFIX="$INSTALL_DIR"
    else
        make install PREFIX="$INSTALL_DIR"
    fi
    cd /
    rm -rf "$tmpdir"
}

install_from_binary() {
    local os="$1"
    local arch
    arch=$(uname -m)

    case "$arch" in
        x86_64|amd64) arch="x86_64" ;;
        aarch64|arm64) arch="arm64" ;;
        *) echo "Unsupported architecture: $arch, falling back to source build"; return 1 ;;
    esac

    local release_url="$ILMA_REPO/releases/download/v${ILMA_VERSION}/ilma-${ILMA_VERSION}-${os}-${arch}.tar.gz"
    local tmpdir
    tmpdir=$(mktemp -d)

    echo -e "${BLUE}Downloading ILMA ${ILMA_VERSION} for ${os}/${arch}...${NC}"
    if curl -fsSL "$release_url" -o "$tmpdir/ilma.tar.gz" 2>/dev/null; then
        tar -xzf "$tmpdir/ilma.tar.gz" -C "$tmpdir"
        if [[ "$EUID" -ne 0 ]]; then
            sudo mkdir -p "$BIN_DIR" "$LIB_DIR"
            sudo cp "$tmpdir/ilma-${ILMA_VERSION}/bin/ilma" "$BIN_DIR/ilma"
            sudo chmod +x "$BIN_DIR/ilma"
            sudo cp "$tmpdir/ilma-${ILMA_VERSION}/lib/ilma/runtime/"* "$LIB_DIR/"
        else
            mkdir -p "$BIN_DIR" "$LIB_DIR"
            cp "$tmpdir/ilma-${ILMA_VERSION}/bin/ilma" "$BIN_DIR/ilma"
            chmod +x "$BIN_DIR/ilma"
            cp "$tmpdir/ilma-${ILMA_VERSION}/lib/ilma/runtime/"* "$LIB_DIR/"
        fi
        rm -rf "$tmpdir"
        return 0
    else
        echo -e "${YELLOW}Binary release not found. Building from source...${NC}"
        rm -rf "$tmpdir"
        return 1
    fi
}

verify_installation() {
    if command -v ilma &>/dev/null; then
        local version
        version=$(ilma --version 2>/dev/null || echo "unknown")
        echo -e "${GREEN}✓ ILMA installed: $version${NC}"
        return 0
    else
        echo -e "${RED}Installation verification failed. 'ilma' not found in PATH.${NC}"
        echo "You may need to add $BIN_DIR to your PATH:"
        echo "  export PATH=\"$BIN_DIR:\$PATH\""
        return 1
    fi
}

main() {
    print_banner

    local os
    os=$(detect_os)

    if [[ "$os" == "windows" ]]; then
        echo -e "${YELLOW}Windows detected. Please use WSL (Windows Subsystem for Linux)${NC}"
        echo "or download a binary release from: $ILMA_REPO/releases"
        exit 1
    fi

    echo -e "${BLUE}Checking dependencies...${NC}"
    check_dependency "gcc"
    check_dependency "git"
    check_dependency "make"

    # Try binary release first, fall back to source
    if ! install_from_binary "$os"; then
        install_from_source
    fi

    echo ""
    verify_installation

    echo ""
    echo -e "${GREEN}${BOLD}ILMA is ready!${NC}"
    echo ""
    echo "Get started:"
    echo -e "  ${BOLD}echo 'say \"Bismillah\"' > hello.ilma${NC}"
    echo -e "  ${BOLD}ilma hello.ilma${NC}"
    echo ""
    echo "Learn more:"
    echo "  ilma-lang.dev"
    echo "  github.com/ilmalang/ilma"
    echo ""
}

main "$@"
