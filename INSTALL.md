# Installation Guide for gleditor

Complete installation instructions for Linux, macOS, and Windows.

---

## 📋 Table of Contents

- [Quick Start](#quick-start)
- [Linux Installation](#linux-installation)
- [macOS Installation](#macos-installation)
- [Windows Installation](#windows-installation)
- [Building from Source](#building-from-source)
- [Troubleshooting](#troubleshooting)

---

## 🚀 Quick Start

### Automated Installation (Linux/macOS)

```bash
git clone https://github.com/yourusername/gleditor.git
cd gleditor
./install.sh
```

That's it! The script will:
- ✅ Detect your OS and package manager
- ✅ Install all required dependencies
- ✅ Build the application
- ✅ Install to `/usr/local/bin`
- ✅ Set up desktop integration (Linux)

### Manual Installation

If you prefer to do it manually or the script doesn't work, see the platform-specific sections below.

---

## 🐧 Linux Installation

### Prerequisites

You need:
- **GTK+3** (>= 3.20)
- **GTKSourceView 4**
- **OpenGL ES 2.0+** and **EGL**
- **GCC** or **Clang**
- **make** or **CMake**
- **pkg-config**

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt install build-essential pkg-config \
    libgtk-3-dev libgtksourceview-4-dev \
    libegl1-mesa-dev libgles2-mesa-dev

# Clone and build
git clone https://github.com/yourusername/gleditor.git
cd gleditor

# Option 1: Use automated installer
./install.sh

# Option 2: Manual build with Make
make
sudo make install

# Option 3: Manual build with CMake
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

### Fedora/RHEL/CentOS

```bash
# Install dependencies
sudo dnf install gcc make pkg-config \
    gtk3-devel gtksourceview4-devel \
    mesa-libEGL-devel mesa-libGLESv2-devel

# Clone and build
git clone https://github.com/yourusername/gleditor.git
cd gleditor
./install.sh
```

### Arch Linux

```bash
# Install dependencies
sudo pacman -S base-devel gtk3 gtksourceview4 mesa

# Clone and build
git clone https://github.com/yourusername/gleditor.git
cd gleditor
./install.sh
```

### openSUSE

```bash
# Install dependencies
sudo zypper install gcc make pkg-config \
    gtk3-devel gtksourceview-devel \
    Mesa-libEGL-devel Mesa-libGLESv2-devel

# Clone and build
git clone https://github.com/yourusername/gleditor.git
cd gleditor
./install.sh
```

### Post-Installation (Linux)

After installation, you can:

1. **Run from terminal:**
   ```bash
   gleditor
   ```

2. **Launch from Application Menu:**
   - Look under **Development** or **Graphics**
   - Search for "gleditor"

3. **Configuration files:**
   - Stored in `~/.config/gleditor/`
   - Settings: `~/.config/gleditor/settings.conf`
   - Session: `~/.config/gleditor/tabs_session.ini`

---

## 🍎 macOS Installation

### Prerequisites

You need **Homebrew** package manager. Install from [brew.sh](https://brew.sh) if you don't have it:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### Installation Steps

```bash
# Install dependencies
brew install gtk+3 gtksourceview4 pkg-config

# Install Xcode Command Line Tools (if not already installed)
xcode-select --install

# Clone and build
git clone https://github.com/yourusername/gleditor.git
cd gleditor
./install.sh
```

### macOS Notes

- **OpenGL on macOS:** Uses desktop OpenGL with ES compatibility layer (macOS doesn't natively support OpenGL ES)
- **App Bundle:** CMake will create a proper macOS app bundle in `/Applications` (if using CMake build)
- **Configuration:** Stored in `~/Library/Application Support/gleditor/`

### Running gleditor on macOS

```bash
# From terminal
gleditor

# Or if installed as app bundle
open /Applications/gleditor.app
```

---

## 🪟 Windows Installation

### Prerequisites

**gleditor** requires **MSYS2** (MinGW environment) for building on Windows.

### Step 1: Install MSYS2

1. Download from [https://www.msys2.org/](https://www.msys2.org/)
2. Run the installer
3. Follow installation wizard (default options are fine)

### Step 2: Install Dependencies

Open **MSYS2 MinGW 64-bit** terminal and run:

```bash
# Update package database
pacman -Syu

# Install build tools
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-make \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-pkg-config

# Install GTK and dependencies
pacman -S mingw-w64-x86_64-gtk3 \
          mingw-w64-x86_64-gtksourceview4
```

### Step 3: Build gleditor

```bash
# Clone repository
git clone https://github.com/yourusername/gleditor.git
cd gleditor

# Build using CMake
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release

# Or use the batch installer
# (in Windows Command Prompt, not MSYS2)
install.bat
```

### Step 4: Running on Windows

```bash
# From MSYS2 MinGW terminal
cd gleditor/bin
./gleditor.exe

# Or double-click gleditor.exe in File Explorer
```

### Windows Notes

- **DLLs Required:** The executable needs GTK DLLs in the same directory or in PATH
- **Configuration:** Stored in `%APPDATA%\gleditor\`
- **Portable Installation:** Copy `bin/` folder with all DLLs to any location

---

## 🔨 Building from Source

### Build System Comparison

gleditor supports **two build systems**. Choose based on your platform and preference:

| Build System | Best For | Pros | Cons |
|--------------|----------|------|------|
| **Make** | Linux, quick builds | ✅ Fast (no configure step)<br/>✅ Simple commands<br/>✅ No CMake dependency<br/>✅ Traditional Unix workflow | ❌ Linux/macOS only<br/>❌ Less IDE integration |
| **CMake** | Windows, macOS, cross-platform | ✅ Cross-platform<br/>✅ IDE support (CLion, VS Code)<br/>✅ Modern standard<br/>✅ Package manager friendly | ❌ Requires CMake<br/>❌ Slower (configure step) |

**Recommendation:**
- **Linux users:** Use Make for simplicity
- **macOS users:** Use CMake (better toolchain integration)
- **Windows users:** Use CMake (required)
- **Contributors:** Test both to ensure they stay in sync

---

### Using Make (Linux/macOS)

```bash
# Clean previous builds
make clean

# Build
make

# Install (requires sudo/root)
sudo make install

# Install to custom location
make install PREFIX=/opt/gleditor

# Build with debug symbols
make debug
```

### Using CMake (All Platforms)

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release ..

# Or with custom install prefix
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/gleditor ..

# Build
cmake --build . --config Release

# Install
sudo cmake --install .

# Or install to custom location
cmake --install . --prefix /opt/gleditor
```

### Build Options

**Makefile targets:**
```bash
make              # Build the application
make clean        # Remove build artifacts
make install      # Install to system
make uninstall    # Remove from system
make run          # Build and run
make debug        # Build with debug symbols
make info         # Show OpenGL ES detection info
```

**CMake options:**
```bash
-DCMAKE_BUILD_TYPE=Release     # Release build (optimized)
-DCMAKE_BUILD_TYPE=Debug       # Debug build (with symbols)
-DCMAKE_INSTALL_PREFIX=/path   # Install location
```

---

## 🐛 Troubleshooting

### "GTK+3 not found"

**Linux:**
```bash
# Debian/Ubuntu
sudo apt install libgtk-3-dev

# Fedora
sudo dnf install gtk3-devel

# Arch
sudo pacman -S gtk3
```

**macOS:**
```bash
brew install gtk+3
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-gtk3
```

---

### "GTKSourceView 4 not found"

**Linux:**
```bash
# Debian/Ubuntu
sudo apt install libgtksourceview-4-dev

# Fedora
sudo dnf install gtksourceview4-devel

# Arch
sudo pacman -S gtksourceview4
```

**macOS:**
```bash
brew install gtksourceview4
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-gtksourceview4
```

---

### "OpenGL ES not found" (Linux)

```bash
# Debian/Ubuntu
sudo apt install libegl1-mesa-dev libgles2-mesa-dev

# Fedora
sudo dnf install mesa-libEGL-devel mesa-libGLESv2-devel

# Arch
sudo pacman -S mesa
```

---

### "Build fails with compiler errors"

1. **Check GCC/Clang version:**
   ```bash
   gcc --version  # Should be 7.0+
   ```

2. **Update build tools:**
   ```bash
   # Debian/Ubuntu
   sudo apt update && sudo apt upgrade build-essential
   
   # Fedora
   sudo dnf update gcc make
   ```

3. **Try CMake instead of Make** (or vice versa)

---

### "gleditor: command not found" after installation

1. **Check if installed:**
   ```bash
   which gleditor
   ls /usr/local/bin/gleditor
   ```

2. **Update PATH:**
   ```bash
   export PATH=/usr/local/bin:$PATH
   # Add to ~/.bashrc or ~/.zshrc for permanence
   ```

3. **Reinstall with verbose output:**
   ```bash
   sudo make install V=1
   ```

---

### Performance Issues / Low FPS

1. **Check GPU acceleration:**
   ```bash
   glxinfo | grep "OpenGL"  # Linux
   ```

2. **Update graphics drivers**

3. **Disable auto-compile** in settings for complex shaders

4. **Reduce shader complexity** (fewer iterations, simpler effects)

---

### "Cannot create config directory" error

**Linux/macOS:**
```bash
mkdir -p ~/.config/gleditor
chmod 755 ~/.config/gleditor
```

**Windows:**
```cmd
mkdir %APPDATA%\gleditor
```

---

### Desktop icon not appearing (Linux)

```bash
# Update desktop database
sudo update-desktop-database /usr/local/share/applications

# Update icon cache
sudo gtk-update-icon-cache /usr/local/share/icons/hicolor
```

---

## 📦 Uninstallation

### Linux/macOS

**If installed with Make:**
```bash
cd gleditor
sudo make uninstall
```

**If installed with CMake:**
```bash
cd gleditor/build
sudo cmake --build . --target uninstall
# Or manually:
sudo rm /usr/local/bin/gleditor
sudo rm -rf /usr/local/share/gleditor
sudo rm /usr/local/share/applications/gleditor.desktop  # Linux only
```

**Remove configuration:**
```bash
# Linux
rm -rf ~/.config/gleditor

# macOS
rm -rf ~/Library/Application\ Support/gleditor
```

### Windows

1. Delete the `gleditor` folder
2. Remove configuration:
   ```cmd
   rmdir /s %APPDATA%\gleditor
   ```

---

## 📚 Additional Resources

- **GitHub Issues:** [Report bugs or request features](https://github.com/yourusername/gleditor/issues)
- **README:** See [README.md](README.md) for usage guide
- **Shadertoy:** [shadertoy.com](https://www.shadertoy.com) for shader examples

---

## ✅ Verification

After installation, verify everything works:

```bash
# Check version
gleditor --version

# Run with verbose output
gleditor --verbose

# Check config directory
ls ~/.config/gleditor/  # Linux
ls ~/Library/Application\ Support/gleditor/  # macOS
dir %APPDATA%\gleditor  # Windows
```

If you see the gleditor window with the default shader preview, you're good to go! 🎉

---

**Need help?** Open an issue on GitHub with:
- Your OS and version
- Error messages (if any)
- Output of `gleditor --verbose`
- Output of `pkg-config --modversion gtk+-3.0`
