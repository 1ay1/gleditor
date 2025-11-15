# Cross-Platform Implementation Guide

This document explains how gleditor achieves cross-platform compatibility across Linux, macOS, and Windows.

---

## 🎯 Overview

gleditor is built with cross-platform support from the ground up, allowing it to run on:

- **Linux** (Ubuntu, Debian, Fedora, Arch, openSUSE, etc.)
- **macOS** (10.12+)
- **Windows** (7/8/10/11 via MSYS2)

---

## 🏗️ Architecture

### Platform Compatibility Layer

The core of cross-platform support is the `include/platform_compat.h` header, which provides:

#### Platform Detection
```c
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#endif
```

#### Abstracted Functions
- **Time:** `platform_get_time()` - High-resolution timing
- **Sleep:** `platform_sleep_ms()` - Cross-platform sleep
- **Paths:** `platform_path_join()` - Handle `/` vs `\` separators
- **Directories:** `platform_mkdir_recursive()` - Create nested directories
- **Config:** `platform_get_config_dir()` - OS-specific config locations

### Build Systems

gleditor supports two build systems:

1. **Makefile** - Traditional Unix/Linux builds (simple, fast)
2. **CMake** - Cross-platform builds (Windows, macOS, Linux)

Both are maintained in parallel to support different workflows.

---

## 🔧 Platform-Specific Implementations

### Linux

**Graphics API:** OpenGL ES 2.0/3.0+ with EGL
**Build:** Native GCC/Clang with pkg-config
**Installation:** System-wide via package manager patterns

```bash
Dependencies:
- libgtk-3-dev
- libgtksourceview-4-dev
- libegl1-mesa-dev
- libgles2-mesa-dev

Config location: ~/.config/gleditor/
```

### macOS

**Graphics API:** Desktop OpenGL with ES compatibility macros
**Build:** Clang with Homebrew dependencies
**Installation:** App bundle or /usr/local/bin

```bash
Dependencies (via Homebrew):
- gtk+3
- gtksourceview4

Config location: ~/Library/Application Support/gleditor/
```

**Key Differences:**
- macOS doesn't natively support OpenGL ES
- Uses `#define` macros to map GLES functions to desktop GL
- App bundle creation via CMake for native macOS experience

### Windows

**Graphics API:** Desktop OpenGL with ES compatibility (via ANGLE or native)
**Build:** MinGW-w64 via MSYS2
**Installation:** Portable executable with bundled DLLs

```bash
Dependencies (via MSYS2/pacman):
- mingw-w64-x86_64-gtk3
- mingw-w64-x86_64-gtksourceview4

Config location: %APPDATA%\gleditor\
```

**Key Differences:**
- Requires MSYS2 for POSIX compatibility layer
- Uses `_mkdir()` instead of `mkdir()`
- Path separators are backslashes
- DLL dependencies must be bundled or in PATH

---

## 📁 File Paths

### Directory Separators

```c
// Linux/macOS
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"

// Windows
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
```

### Config Directories

| Platform | Location |
|----------|----------|
| Linux | `~/.config/gleditor/` |
| macOS | `~/Library/Application Support/gleditor/` |
| Windows | `%APPDATA%\gleditor\` |

### Usage Example

```c
char config_path[PATH_MAX];
platform_get_config_dir(config_path, sizeof(config_path));
// Linux:   /home/user/.config/gleditor
// macOS:   /Users/user/Library/Application Support/gleditor
// Windows: C:\Users\user\AppData\Roaming\gleditor
```

---

## ⏱️ Time Functions

### Problem: Different Time APIs

- **Linux/macOS:** `gettimeofday()` from `<sys/time.h>`
- **Windows:** `QueryPerformanceCounter()` from `<windows.h>`

### Solution: Unified Interface

```c
double platform_get_time(void) {
#ifdef PLATFORM_WINDOWS
    static LARGE_INTEGER frequency;
    static bool initialized = false;
    
    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = true;
    }
    
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
#endif
}
```

**Usage in code:**
```c
double start_time = platform_get_time();
// ... render shader ...
double elapsed = platform_get_time() - start_time;
```

---

## 🎨 OpenGL ES vs OpenGL

### Challenge

Different platforms support different OpenGL variants:

| Platform | Native API | Solution |
|----------|------------|----------|
| Linux | OpenGL ES 2.0/3.0+ | Use directly |
| macOS | Desktop OpenGL 3.2+ | Compatibility layer |
| Windows | Desktop OpenGL | Compatibility layer |

### Compatibility Layer

For macOS and Windows, we use preprocessor macros:

```c
#if defined(USE_OPENGL_COMPAT)
    // Map GLES functions to desktop GL equivalents
    #define glClearDepthf glClearDepth
    #define glDepthRangef glDepthRange
    // ... etc
#endif
```

This allows the same shader code to compile on all platforms.

---

## 📦 Installation

### Automated Installers

#### Linux/macOS: `install.sh`

Features:
- ✅ Detects OS and package manager (apt, dnf, pacman, brew)
- ✅ Installs dependencies automatically
- ✅ Builds with optimal flags
- ✅ Installs system-wide with sudo
- ✅ Sets up desktop integration (Linux)

```bash
./install.sh
# Handles everything automatically
```

#### Windows: `install.bat`

Features:
- ✅ Checks for MSYS2 environment
- ✅ Validates dependencies
- ✅ Builds with MinGW
- ✅ Bundles required DLLs
- ✅ Creates portable installation

```batch
install.bat
REM Guides user through MSYS2 setup if needed
```

### Build Systems

#### Makefile (Linux/macOS)

```bash
make              # Build
make install      # Install to /usr/local
make PREFIX=/opt  # Custom install location
```

Features:
- Automatic OpenGL ES version detection
- pkg-config integration
- Parallel builds with -j
- Debug/release modes

#### CMake (All Platforms)

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cmake --install .
```

Features:
- Cross-platform configuration
- Automatic dependency finding
- Out-of-source builds
- Install target support
- App bundle creation (macOS)

---

## 🔍 Platform-Specific Code Patterns

### File Operations

```c
// Include platform_compat.h
#include "platform_compat.h"

// Cross-platform file check
if (platform_file_exists("shader.glsl")) {
    // Works on all platforms
}

// Cross-platform directory creation
platform_mkdir_recursive(config_dir);
```

### Path Joining

```c
char full_path[PATH_MAX];
platform_path_join(full_path, sizeof(full_path), 
                   home_dir, "shaders/example.glsl");

// Linux:   /home/user/shaders/example.glsl
// macOS:   /Users/user/shaders/example.glsl
// Windows: C:\Users\user\shaders\example.glsl
```

### Config Directory

```c
char config_dir[PATH_MAX];
platform_get_config_dir(config_dir, sizeof(config_dir));

// Now safe to use on any platform
platform_mkdir_recursive(config_dir);
```

---

## 🧪 Testing Across Platforms

### Verification Checklist

- [ ] **Linux:** Test on Ubuntu, Fedora, Arch
- [ ] **macOS:** Test on both Intel and ARM (M1/M2)
- [ ] **Windows:** Test with MSYS2 MinGW 64-bit
- [ ] **Paths:** Verify config files created in correct locations
- [ ] **OpenGL:** Check shader compilation on all platforms
- [ ] **Install:** Verify both automated and manual installation
- [ ] **Desktop:** Check application menu integration (Linux)

### Common Issues

| Issue | Platform | Solution |
|-------|----------|----------|
| Missing OpenGL ES | macOS/Windows | Use compatibility layer |
| Path separator errors | Windows | Use `platform_path_join()` |
| DLL not found | Windows | Bundle DLLs or add to PATH |
| Permission denied | Linux/macOS | Check install prefix permissions |
| GTK not found | All | Install development packages |

---

## 📝 Developer Guidelines

### Adding New Platform-Specific Code

1. **Add to `platform_compat.h`** if it's a common operation
2. **Use preprocessor guards** for platform-specific includes
3. **Test on all three platforms** before committing
4. **Document** any new platform differences

### Example: Adding New Feature

```c
// Good: Platform-agnostic
char path[PATH_MAX];
platform_path_join(path, sizeof(path), dir, "file.txt");

// Bad: Linux-only
char path[PATH_MAX];
snprintf(path, sizeof(path), "%s/file.txt", dir);  // Breaks on Windows!
```

### Code Review Checklist

- [ ] No hardcoded `/` or `\` in paths
- [ ] No POSIX-only functions (`unistd.h`) without guards
- [ ] Time functions use `platform_get_time()`
- [ ] File operations use platform compatibility layer
- [ ] Tested on Linux AND (macOS OR Windows)

---

## 🚀 Future Improvements

### Planned

- [ ] **Native Windows build** (MSVC without MSYS2)
- [ ] **Wayland support** (Linux)
- [ ] **ARM64 optimization** (macOS M1/M2, Linux ARM)
- [ ] **Snap/Flatpak packages** (Linux distribution)
- [ ] **Homebrew formula** (macOS easy install)
- [ ] **Chocolatey package** (Windows package manager)

### Possible

- [ ] **BSD support** (FreeBSD, OpenBSD)
- [ ] **Android port** (via GTK on Android)
- [ ] **Web build** (via Emscripten + WebGL)

---

## 📚 References

- **Platform Detection:** CMake documentation, compiler predefined macros
- **GTK Cross-Platform:** [GTK.org Platform Support](https://www.gtk.org/)
- **OpenGL ES/GL Compat:** [Khronos OpenGL Wiki](https://www.khronos.org/opengl/wiki)
- **MSYS2:** [msys2.org](https://www.msys2.org/)
- **Homebrew:** [brew.sh](https://brew.sh/)

---

## 🤝 Contributing

When adding features, remember:

- **Test on multiple platforms** (at minimum: Linux + one other)
- **Use platform abstraction** from `platform_compat.h`
- **Update installers** if dependencies change
- **Document** platform-specific behavior
- **Check CMake AND Makefile** both work

---

**Questions?** Check [INSTALL.md](../INSTALL.md) for installation details or open an issue on GitHub.