#!/bin/bash
# gleditor Build Script
# Builds the application without installing (just compile)

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
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "mingw"* ]]; then
    OS="windows"
fi

echo -e "${BLUE}╔═══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║        gleditor Build Script          ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════╝${NC}"
echo ""
echo -e "${GREEN}Platform: ${OS}${NC}"
echo ""

# Parse command line arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false
USE_CMAKE=false
USE_MAKE=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --cmake)
            USE_CMAKE=true
            shift
            ;;
        --make)
            USE_MAKE=true
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --debug         Build with debug symbols (default: Release)"
            echo "  --clean         Clean before building"
            echo "  --cmake         Force use of CMake"
            echo "  --make          Force use of Make"
            echo "  --verbose, -v   Verbose output"
            echo "  --help, -h      Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                  # Quick build (auto-detect best system)"
            echo "  $0 --debug          # Debug build"
            echo "  $0 --clean --cmake  # Clean CMake build"
            echo "  $0 --make -v        # Verbose Make build"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Determine number of CPU cores
if [ "$OS" == "macos" ]; then
    NCORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 2)
else
    NCORES=$(nproc 2>/dev/null || echo 2)
fi

# Auto-select build system if not forced
if [ "$USE_CMAKE" = false ] && [ "$USE_MAKE" = false ]; then
    if [ "$OS" == "linux" ]; then
        # Linux: prefer Make
        if [ -f "Makefile" ]; then
            USE_MAKE=true
        elif command_exists cmake; then
            USE_CMAKE=true
        fi
    elif [ "$OS" == "macos" ]; then
        # macOS: prefer CMake
        if command_exists cmake; then
            USE_CMAKE=true
        elif [ -f "Makefile" ]; then
            USE_MAKE=true
        fi
    else
        # Windows/unknown: prefer CMake
        if command_exists cmake; then
            USE_CMAKE=true
        elif [ -f "Makefile" ]; then
            USE_MAKE=true
        fi
    fi
fi

# Build with Make
if [ "$USE_MAKE" = true ]; then
    echo -e "${BLUE}🔨 Building with Make...${NC}"
    echo -e "${YELLOW}Build type: ${BUILD_TYPE}${NC}"
    echo ""

    if [ ! -f "Makefile" ]; then
        echo -e "${RED}❌ Makefile not found!${NC}"
        exit 1
    fi

    # Clean if requested
    if [ "$CLEAN_BUILD" = true ]; then
        echo -e "${BLUE}Cleaning previous build...${NC}"
        make clean
    fi

    # Build
    if [ "$BUILD_TYPE" == "Debug" ]; then
        MAKE_TARGET="debug"
    else
        MAKE_TARGET=""
    fi

    if [ "$VERBOSE" = true ]; then
        make $MAKE_TARGET V=1 -j${NCORES}
    else
        make $MAKE_TARGET -j${NCORES}
    fi

    if [ $? -eq 0 ]; then
        echo ""
        echo -e "${GREEN}✓ Build successful!${NC}"
        echo -e "${BLUE}Binary: ${YELLOW}bin/gleditor${NC}"
        echo ""
        echo -e "Run with: ${YELLOW}./bin/gleditor${NC}"
    else
        echo -e "${RED}❌ Build failed!${NC}"
        exit 1
    fi

# Build with CMake
elif [ "$USE_CMAKE" = true ]; then
    echo -e "${BLUE}🔨 Building with CMake...${NC}"
    echo -e "${YELLOW}Build type: ${BUILD_TYPE}${NC}"
    echo ""

    if ! command_exists cmake; then
        echo -e "${RED}❌ CMake not found!${NC}"
        if [ "$OS" == "macos" ]; then
            echo -e "${YELLOW}Install with: brew install cmake${NC}"
        elif [ "$OS" == "linux" ]; then
            echo -e "${YELLOW}Install with: sudo apt install cmake  (Ubuntu/Debian)${NC}"
            echo -e "${YELLOW}           or: sudo dnf install cmake  (Fedora)${NC}"
            echo -e "${YELLOW}           or: sudo pacman -S cmake    (Arch)${NC}"
        fi
        exit 1
    fi

    # Clean if requested
    if [ "$CLEAN_BUILD" = true ]; then
        echo -e "${BLUE}Cleaning previous build...${NC}"
        rm -rf build
    fi

    # Create build directory
    mkdir -p build
    cd build

    # Configure
    echo -e "${BLUE}Configuring...${NC}"
    if [ "$VERBOSE" = true ]; then
        cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_VERBOSE_MAKEFILE=ON ..
    else
        cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} .. > /dev/null
    fi

    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ CMake configuration failed!${NC}"
        cd ..
        exit 1
    fi

    # Build
    echo -e "${BLUE}Building...${NC}"
    if [ "$VERBOSE" = true ]; then
        cmake --build . --config ${BUILD_TYPE} -- -j${NCORES}
    else
        cmake --build . --config ${BUILD_TYPE} -- -j${NCORES} 2>&1 | grep -i "error" || true
    fi

    if [ $? -eq 0 ]; then
        cd ..
        echo ""
        echo -e "${GREEN}✓ Build successful!${NC}"

        # Find the binary
        if [ -f "build/gleditor" ]; then
            echo -e "${BLUE}Binary: ${YELLOW}build/gleditor${NC}"
            echo ""
            echo -e "Run with: ${YELLOW}./build/gleditor${NC}"
        elif [ -f "bin/gleditor" ]; then
            echo -e "${BLUE}Binary: ${YELLOW}bin/gleditor${NC}"
            echo ""
            echo -e "Run with: ${YELLOW}./bin/gleditor${NC}"
        else
            echo -e "${YELLOW}Binary location may vary${NC}"
        fi
    else
        cd ..
        echo -e "${RED}❌ Build failed!${NC}"
        exit 1
    fi

else
    echo -e "${RED}❌ No build system available!${NC}"
    echo ""
    echo "Please install one of the following:"
    echo "  - make (for Make-based builds)"
    echo "  - cmake (for CMake-based builds)"
    exit 1
fi

echo ""
echo -e "${GREEN}╔═══════════════════════════════════════╗${NC}"
echo -e "${GREEN}║     Build Complete! 🎉                ║${NC}"
echo -e "${GREEN}╚═══════════════════════════════════════╝${NC}"
echo ""

if [ "$BUILD_TYPE" == "Debug" ]; then
    echo -e "${YELLOW}Note: Debug build includes symbols and is slower${NC}"
    echo -e "${YELLOW}For production use, build without --debug${NC}"
    echo ""
fi
