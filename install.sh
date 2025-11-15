#!/bin/bash
# gleditor Installation Script
# Easy installation for Linux and macOS

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Detect OS
OS="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
fi

echo -e "${BLUE}╔═══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     gleditor Installation Script     ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════╝${NC}"
echo ""
echo -e "${GREEN}Platform detected: ${OS}${NC}"
echo ""

# Check if running as root (we don't want that)
if [ "$EUID" -eq 0 ]; then
   echo -e "${RED}❌ Please don't run this script as root!${NC}"
   echo -e "${YELLOW}Run: ./install.sh${NC}"
   exit 1
fi

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for required build tools
echo -e "${BLUE}🔍 Checking for required tools...${NC}"

MISSING_DEPS=0

if ! command_exists gcc && ! command_exists clang; then
    echo -e "${RED}❌ No C compiler found (gcc or clang)${NC}"
    MISSING_DEPS=1
fi

if ! command_exists make; then
    echo -e "${RED}❌ make not found${NC}"
    MISSING_DEPS=1
fi

if ! command_exists pkg-config; then
    echo -e "${RED}❌ pkg-config not found${NC}"
    MISSING_DEPS=1
fi

if [ $MISSING_DEPS -eq 1 ]; then
    echo ""
    echo -e "${YELLOW}📦 Installing missing dependencies...${NC}"

    if [ "$OS" == "linux" ]; then
        if command_exists apt-get; then
            echo -e "${BLUE}Using apt-get...${NC}"
            sudo apt-get update
            sudo apt-get install -y build-essential pkg-config
        elif command_exists dnf; then
            echo -e "${BLUE}Using dnf...${NC}"
            sudo dnf install -y gcc make pkg-config
        elif command_exists pacman; then
            echo -e "${BLUE}Using pacman...${NC}"
            sudo pacman -S --needed base-devel
        else
            echo -e "${RED}❌ Unknown package manager. Please install build tools manually.${NC}"
            exit 1
        fi
    elif [ "$OS" == "macos" ]; then
        if command_exists brew; then
            echo -e "${BLUE}Using Homebrew...${NC}"
            brew install pkg-config
        else
            echo -e "${YELLOW}⚠️  Homebrew not found. Install from https://brew.sh${NC}"
            echo -e "${YELLOW}Then install Xcode Command Line Tools: xcode-select --install${NC}"
            exit 1
        fi
    fi
fi

# Check for GTK and other dependencies
echo -e "${BLUE}🔍 Checking for GTK+3 and dependencies...${NC}"

if ! pkg-config --exists gtk+-3.0; then
    echo -e "${YELLOW}📦 GTK+3 not found. Installing...${NC}"

    if [ "$OS" == "linux" ]; then
        if command_exists apt-get; then
            sudo apt-get install -y libgtk-3-dev libgtksourceview-4-dev \
                libegl1-mesa-dev libgles2-mesa-dev
        elif command_exists dnf; then
            sudo dnf install -y gtk3-devel gtksourceview4-devel \
                mesa-libEGL-devel mesa-libGLESv2-devel
        elif command_exists pacman; then
            sudo pacman -S --needed gtk3 gtksourceview4 mesa
        fi
    elif [ "$OS" == "macos" ]; then
        if command_exists brew; then
            brew install gtk+3 gtksourceview4
        fi
    fi
fi

# Verify dependencies are now available
echo -e "${BLUE}✓ Verifying dependencies...${NC}"

if pkg-config --exists gtk+-3.0; then
    GTK_VERSION=$(pkg-config --modversion gtk+-3.0)
    echo -e "${GREEN}  ✓ GTK+3: ${GTK_VERSION}${NC}"
else
    echo -e "${RED}  ❌ GTK+3 still not found${NC}"
    exit 1
fi

if pkg-config --exists gtksourceview-4; then
    GTKSV_VERSION=$(pkg-config --modversion gtksourceview-4)
    echo -e "${GREEN}  ✓ GTKSourceView 4: ${GTKSV_VERSION}${NC}"
else
    echo -e "${RED}  ❌ GTKSourceView 4 not found${NC}"
    exit 1
fi

if [ "$OS" == "linux" ]; then
    if pkg-config --exists egl glesv2; then
        echo -e "${GREEN}  ✓ OpenGL ES + EGL${NC}"
    else
        echo -e "${YELLOW}  ⚠️  OpenGL ES not found, but might work anyway${NC}"
    fi
fi

echo ""

# Build the application
echo -e "${BLUE}🔨 Building gleditor...${NC}"

if [ -f "CMakeLists.txt" ]; then
    # Use CMake if available
    if command_exists cmake; then
        echo -e "${GREEN}Using CMake build system${NC}"
        mkdir -p build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Release ..
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
        cd ..
    else
        echo -e "${YELLOW}CMake not found, falling back to Makefile${NC}"
        make clean
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    fi
else
    # Use Makefile
    make clean
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
fi

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build successful!${NC}"
echo ""

# Install
echo -e "${BLUE}📦 Installing gleditor...${NC}"

# Determine install prefix
if [ -w "/usr/local/bin" ]; then
    PREFIX="/usr/local"
    NEED_SUDO=false
else
    PREFIX="/usr/local"
    NEED_SUDO=true
fi

echo -e "${YELLOW}Install location: ${PREFIX}${NC}"

if [ "$NEED_SUDO" = true ]; then
    echo -e "${YELLOW}This requires sudo access...${NC}"
    if [ -f "build/Makefile" ]; then
        sudo cmake --install build --prefix "$PREFIX"
    else
        sudo make install PREFIX="$PREFIX"
    fi
else
    if [ -f "build/Makefile" ]; then
        cmake --install build --prefix "$PREFIX"
    else
        make install PREFIX="$PREFIX"
    fi
fi

# Install data files
echo -e "${BLUE}📄 Installing data files...${NC}"

DATA_DIR="$PREFIX/share/gleditor"
if [ "$NEED_SUDO" = true ]; then
    sudo mkdir -p "$DATA_DIR"
    sudo cp -r data/*.svg "$DATA_DIR/" 2>/dev/null || true
else
    mkdir -p "$DATA_DIR"
    cp -r data/*.svg "$DATA_DIR/" 2>/dev/null || true
fi

# Install desktop entry (Linux only)
if [ "$OS" == "linux" ]; then
    echo -e "${BLUE}🖥️  Installing desktop entry...${NC}"

    DESKTOP_DIR="$PREFIX/share/applications"
    ICON_DIR="$PREFIX/share/icons/hicolor/scalable/apps"

    if [ "$NEED_SUDO" = true ]; then
        sudo mkdir -p "$DESKTOP_DIR"
        sudo mkdir -p "$ICON_DIR"
        sudo cp data/gleditor.desktop "$DESKTOP_DIR/" 2>/dev/null || true
        sudo cp data/icon.svg "$ICON_DIR/gleditor.svg" 2>/dev/null || true
    else
        mkdir -p "$DESKTOP_DIR"
        mkdir -p "$ICON_DIR"
        cp data/gleditor.desktop "$DESKTOP_DIR/" 2>/dev/null || true
        cp data/icon.svg "$ICON_DIR/gleditor.svg" 2>/dev/null || true
    fi

    # Update desktop database
    if command_exists update-desktop-database; then
        if [ "$NEED_SUDO" = true ]; then
            sudo update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
        else
            update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
        fi
    fi
fi

echo ""
echo -e "${GREEN}╔═══════════════════════════════════════╗${NC}"
echo -e "${GREEN}║   ✓ Installation Complete!           ║${NC}"
echo -e "${GREEN}╚═══════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}To run gleditor:${NC}"
echo -e "  ${YELLOW}gleditor${NC}"
echo ""

if [ "$OS" == "linux" ]; then
    echo -e "${BLUE}You can also find it in your application menu under Development > gleditor${NC}"
    echo ""
fi

echo -e "${BLUE}Configuration files will be saved to:${NC}"
if [ "$OS" == "macos" ]; then
    echo -e "  ${YELLOW}~/Library/Application Support/gleditor/${NC}"
else
    echo -e "  ${YELLOW}~/.config/gleditor/${NC}"
fi

echo ""
echo -e "${GREEN}Happy shader coding! 🎨✨${NC}"
