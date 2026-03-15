# ILMA Programming Language Installer for Windows
# Usage: irm https://ilma-lang.dev/install.ps1 | iex
#
# This script downloads and installs the ILMA compiler on Windows.
# It requires GCC (MinGW-w64) for compiling ILMA programs.
#
# Environment:
#   ILMA_INSTALL_DIR  — Override install directory (default: %LOCALAPPDATA%\ilma)

$ErrorActionPreference = "Stop"

$ILMA_VERSION = "0.5.0"
$ILMA_REPO = "https://github.com/raihan-js/ilma-lang"
$INSTALL_DIR = if ($env:ILMA_INSTALL_DIR) { $env:ILMA_INSTALL_DIR } else { "$env:LOCALAPPDATA\ilma" }
$BIN_DIR = "$INSTALL_DIR\bin"
$LIB_DIR = "$INSTALL_DIR\lib\ilma\runtime"
$PACKAGES_DIR = "$env:USERPROFILE\.ilma\packages"

# ── Banner ───────────────────────────────────────────────────────────────────

Write-Host ""
Write-Host "  ___ _     __  __    _   " -ForegroundColor Cyan
Write-Host " |_ _| |   |  \/  |  / \  " -ForegroundColor Cyan
Write-Host "  | || |   | |\/| | / _ \ " -ForegroundColor Cyan
Write-Host "  | || |___| |  | |/ ___ \" -ForegroundColor Cyan
Write-Host " |___|_____|_|  |_/_/   \_\" -ForegroundColor Cyan
Write-Host ""
Write-Host "  ILMA Programming Language" -ForegroundColor Blue
Write-Host "  ilma-lang.dev" -ForegroundColor Blue
Write-Host "  Version $ILMA_VERSION"
Write-Host ""

# ── Helper functions ─────────────────────────────────────────────────────────

function Write-Step {
    param([string]$Message)
    Write-Host "  -> $Message" -ForegroundColor Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "  [OK] $Message" -ForegroundColor Green
}

function Write-Warn {
    param([string]$Message)
    Write-Host "  [!] $Message" -ForegroundColor Yellow
}

function Write-Fail {
    param([string]$Message)
    Write-Host "  [X] $Message" -ForegroundColor Red
}

# ── Check prerequisites ─────────────────────────────────────────────────────

Write-Step "Checking prerequisites..."

# Check for GCC
$gcc = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gcc) {
    Write-Fail "GCC not found."
    Write-Host ""
    Write-Host "  ILMA compiles to C, so GCC is required." -ForegroundColor White
    Write-Host ""

    # Try winget first
    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if ($winget) {
        Write-Host "  Install MinGW-w64 via winget:" -ForegroundColor Yellow
        Write-Host "    winget install -e --id MSYS2.MSYS2" -ForegroundColor White
        Write-Host ""
    }

    # Suggest alternatives
    Write-Host "  Other options:" -ForegroundColor Yellow
    Write-Host "    1. Chocolatey:  choco install mingw" -ForegroundColor White
    Write-Host "    2. Scoop:       scoop install gcc" -ForegroundColor White
    Write-Host "    3. Download:    https://www.mingw-w64.org/downloads/" -ForegroundColor White
    Write-Host "    4. WSL:         wsl --install  (recommended for full Linux environment)" -ForegroundColor White
    Write-Host ""
    Write-Host "  After installing GCC, restart your terminal and run this script again." -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

$gccVersion = & gcc --version 2>&1 | Select-Object -First 1
Write-Success "GCC found: $gccVersion"

# ── Create directories ───────────────────────────────────────────────────────

Write-Step "Creating directories..."

New-Item -ItemType Directory -Force -Path $BIN_DIR | Out-Null
New-Item -ItemType Directory -Force -Path $LIB_DIR | Out-Null
New-Item -ItemType Directory -Force -Path $PACKAGES_DIR | Out-Null

Write-Success "Install directory: $INSTALL_DIR"
Write-Success "Package directory: $PACKAGES_DIR"

# ── Download and install ─────────────────────────────────────────────────────

$arch = if ([Environment]::Is64BitOperatingSystem) { "x86_64" } else { "x86" }
$releaseUrl = "$ILMA_REPO/releases/download/v$ILMA_VERSION/ilma-$ILMA_VERSION-windows-$arch.zip"

Write-Step "Downloading ILMA $ILMA_VERSION for windows/$arch..."

$installed = $false

try {
    $tmpZip = "$env:TEMP\ilma-$ILMA_VERSION.zip"
    $tmpDir = "$env:TEMP\ilma-extract-$([System.Guid]::NewGuid().ToString('N').Substring(0,8))"

    # Download the release archive
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $releaseUrl -OutFile $tmpZip -UseBasicParsing

    # Extract
    if (Test-Path $tmpDir) { Remove-Item -Recurse -Force $tmpDir }
    Expand-Archive -Path $tmpZip -DestinationPath $tmpDir

    # Find and copy the ilma.exe binary
    $exe = Get-ChildItem -Path $tmpDir -Recurse -Filter "ilma.exe" | Select-Object -First 1
    if (-not $exe) {
        # On some releases the binary might not have .exe extension
        $exeNoExt = Get-ChildItem -Path $tmpDir -Recurse -Filter "ilma" | Where-Object { -not $_.PSIsContainer } | Select-Object -First 1
        if ($exeNoExt) {
            Copy-Item $exeNoExt.FullName "$BIN_DIR\ilma.exe" -Force
        } else {
            throw "Binary not found in release archive"
        }
    } else {
        Copy-Item $exe.FullName "$BIN_DIR\ilma.exe" -Force
    }

    # Copy runtime files (*.c and *.h)
    $runtimeFiles = Get-ChildItem -Path $tmpDir -Recurse -Include "*.c","*.h"
    foreach ($f in $runtimeFiles) {
        Copy-Item $f.FullName $LIB_DIR -Force
    }

    # Clean up temp files
    Remove-Item -Recurse -Force $tmpDir -ErrorAction SilentlyContinue
    Remove-Item -Force $tmpZip -ErrorAction SilentlyContinue

    Write-Success "Binary release installed."
    $installed = $true
}
catch {
    Write-Warn "Binary release not available. Building from source..."

    # Check for git
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) {
        Write-Fail "git is required to build from source."
        Write-Host ""
        Write-Host "  Install git: https://git-scm.com/download/win" -ForegroundColor White
        Write-Host "  Or:  winget install -e --id Git.Git" -ForegroundColor White
        Write-Host ""
        exit 1
    }

    # Check for make
    $make = Get-Command make -ErrorAction SilentlyContinue
    $mingw32make = Get-Command mingw32-make -ErrorAction SilentlyContinue

    $makeCmd = "make"
    if ($mingw32make) {
        $makeCmd = "mingw32-make"
    } elseif (-not $make) {
        Write-Fail "make (or mingw32-make) is required to build from source."
        Write-Host ""
        Write-Host "  If you installed MinGW, make sure mingw32-make is in your PATH." -ForegroundColor White
        Write-Host "  Or install GNU Make: choco install make" -ForegroundColor White
        Write-Host ""
        exit 1
    }

    $tmpSrc = "$env:TEMP\ilma-src-$([System.Guid]::NewGuid().ToString('N').Substring(0,8))"

    try {
        Write-Step "Cloning repository..."
        & git clone --depth=1 "$ILMA_REPO.git" $tmpSrc 2>&1 | Out-Null

        Write-Step "Building ILMA compiler..."
        Push-Location $tmpSrc
        & $makeCmd all 2>&1 | Out-Null

        # Copy the built binary
        if (Test-Path "build\ilma.exe") {
            Copy-Item "build\ilma.exe" "$BIN_DIR\ilma.exe" -Force
        } elseif (Test-Path "build\ilma") {
            Copy-Item "build\ilma" "$BIN_DIR\ilma.exe" -Force
        } else {
            throw "Build failed: no binary produced in build/"
        }

        # Copy runtime files
        $srcRuntime = Get-ChildItem -Path "src\runtime" -Recurse -Include "*.c","*.h" -ErrorAction SilentlyContinue
        foreach ($f in $srcRuntime) {
            Copy-Item $f.FullName $LIB_DIR -Force
        }

        Pop-Location
        Write-Success "Built from source."
        $installed = $true
    }
    catch {
        Pop-Location -ErrorAction SilentlyContinue
        Write-Fail "Build failed: $_"
        exit 1
    }
    finally {
        Remove-Item -Recurse -Force $tmpSrc -ErrorAction SilentlyContinue
    }
}

if (-not $installed) {
    Write-Fail "Installation failed."
    exit 1
}

# ── Update PATH ──────────────────────────────────────────────────────────────

Write-Step "Configuring PATH..."

$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($currentPath -notlike "*$BIN_DIR*") {
    [Environment]::SetEnvironmentVariable("Path", "$currentPath;$BIN_DIR", "User")
    Write-Success "Added $BIN_DIR to user PATH."
    Write-Warn "Restart your terminal for PATH changes to take effect."
} else {
    Write-Success "$BIN_DIR is already in PATH."
}

# Add to current session PATH so verification works
$env:Path = "$BIN_DIR;$env:Path"

# ── Verify installation ─────────────────────────────────────────────────────

Write-Step "Verifying installation..."

try {
    $ver = & "$BIN_DIR\ilma.exe" --version 2>&1
    Write-Success "Installed: $ver"
}
catch {
    Write-Warn "Could not verify. Run 'ilma --version' in a new terminal."
}

# ── Done ─────────────────────────────────────────────────────────────────────

Write-Host ""
Write-Host "  ============================================" -ForegroundColor Green
Write-Host "    ILMA is ready!" -ForegroundColor Green
Write-Host "  ============================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Get started:" -ForegroundColor White
Write-Host "    echo 'say `"Bismillah`"' > hello.ilma" -ForegroundColor Cyan
Write-Host "    ilma hello.ilma" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Useful commands:" -ForegroundColor White
Write-Host "    ilma --help              Show all options" -ForegroundColor Gray
Write-Host "    ilma --repl              Interactive mode" -ForegroundColor Gray
Write-Host "    ilma get <package>       Install a package" -ForegroundColor Gray
Write-Host "    ilma packages            List installed packages" -ForegroundColor Gray
Write-Host ""
Write-Host "  Learn more:" -ForegroundColor White
Write-Host "    Website:  https://ilma-lang.dev" -ForegroundColor Gray
Write-Host "    GitHub:   https://github.com/raihan-js/ilma-lang" -ForegroundColor Gray
Write-Host "    Spec:     https://github.com/raihan-js/ilma-lang/blob/main/SPEC.md" -ForegroundColor Gray
Write-Host ""
Write-Host "  Uninstall:" -ForegroundColor White
Write-Host "    Remove-Item -Recurse -Force '$INSTALL_DIR'" -ForegroundColor Gray
Write-Host "    # Then remove $BIN_DIR from your PATH in System Settings" -ForegroundColor Gray
Write-Host ""
