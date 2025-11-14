# NeoWall Shader Editor (gleditor)

A standalone OpenGL ES shader editor for creating and installing shaders to NeoWall.

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## Features

- 🎨 **Live Preview** - Real-time shader compilation and rendering
- 📝 **Syntax Highlighting** - GLSL syntax highlighting with GTKSourceView
- 🔧 **OpenGL ES Support** - Automatic detection of ES 2.0, 3.0, 3.1, 3.2
- 🎯 **Shadertoy Compatible** - Supports standard Shadertoy uniforms (`iTime`, `iResolution`, `iMouse`)
- 💾 **Save/Load** - Save and load shader files
- 📦 **NeoWall Integration** - Install shaders directly to NeoWall
- ⚡ **Auto-Compile** - Automatic shader recompilation as you type
- 🖱️ **Mouse Tracking** - Interactive mouse input for shaders

## Screenshots

```
┌─────────────────────────────────────────────────────────┐
│ NeoWall Shader Editor                                   │
├─────────────────────────────────────────────────────────┤
│ [New] [Load] [Save] │ [Compile] [Pause] │ [Install]    │
├───────────────────────┬─────────────────────────────────┤
│ // GLSL Editor        │  ┌─────────────────────────┐   │
│ void mainImage(...)   │  │                         │   │
│ {                     │  │   Live OpenGL Preview   │   │
│   vec2 uv = ...       │  │                         │   │
│   ...                 │  │     [Shader Output]     │   │
│ }                     │  │                         │   │
│                       │  └─────────────────────────┘   │
│ ✓ Compiled OK         │                               │
├───────────────────────┴─────────────────────────────────┤
│ Ready                                    Line 1, Col 1  │
└─────────────────────────────────────────────────────────┘
```

## Requirements

### Build Dependencies

- **GTK+3** (>= 3.20)
- **GTKSourceView 4**
- **OpenGL ES 2.0** (minimum)
- **EGL**
- **GCC** or compatible C compiler
- **pkg-config**

### Optional

- OpenGL ES 3.0+ for enhanced features
- OpenGL ES 3.1 for compute shaders
- OpenGL ES 3.2 for geometry/tessellation shaders

## Installation

### Building from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/yourusername/gleditor.git
   cd gleditor
   ```

2. **Install dependencies:**

   **Debian/Ubuntu:**
   ```bash
   sudo apt install build-essential pkg-config \
       libgtk-3-dev libgtksourceview-4-dev \
       libgles2-mesa-dev libegl1-mesa-dev
   ```

   **Fedora:**
   ```bash
   sudo dnf install gcc gtk3-devel gtksourceview4-devel \
       mesa-libGLES-devel mesa-libEGL-devel
   ```

   **Arch Linux:**
   ```bash
   sudo pacman -S base-devel gtk3 gtksourceview4 \
       mesa glu
   ```

3. **Build:**
   ```bash
   make
   ```

4. **Run:**
   ```bash
   make run
   # or directly:
   ./bin/gleditor
   ```

5. **Install (optional):**
   ```bash
   sudo make install
   # Default prefix: /usr/local
   # Custom prefix:
   sudo make install PREFIX=/usr
   ```

### Build Options

```bash
# Debug build with symbols
make debug

# Check detected OpenGL ES versions
make info

# Clean build artifacts
make clean

# Uninstall
sudo make uninstall
```

## Usage

### Starting the Editor

```bash
gleditor [OPTIONS]
```

**Options:**
- `-v, --version` - Show version information
- `-V, --verbose` - Enable verbose output
- `-h, --help` - Show help message

### Creating a Shader

1. **Start with the default template** or click **New** to reset
2. **Edit the shader** in the left pane
3. **Watch the live preview** in the right pane
4. Shaders auto-compile as you type (500ms debounce)

### Shader Format

Shaders use **Shadertoy-compatible** format:

```glsl
// Your shader code here
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Normalized pixel coordinates (0 to 1)
    vec2 uv = fragCoord / iResolution.xy;
    
    // Time-based animation
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));
    
    // Output
    fragColor = vec4(col, 1.0);
}
```

### Available Uniforms

The editor provides standard Shadertoy uniforms:

- `vec3 iResolution` - Viewport resolution (width, height, aspect)
- `float iTime` - Shader playback time (seconds)
- `float iTimeDelta` - Time since last frame
- `int iFrame` - Shader playback frame number
- `vec4 iMouse` - Mouse position (x, y, click_x, click_y)

### Saving and Loading

- **Save** - Save shader to a `.glsl` file
- **Load** - Load shader from file
- **Auto-save** - No autosave; remember to save your work!

### Installing to NeoWall

1. Click **Install to NeoWall**
2. Enter a shader name (e.g., `my_cool_shader.glsl`)
3. Shader will be copied to `~/.config/neowall/shaders/`
4. Use NeoWall to activate your shader

### Controls

**Toolbar:**
- **New** - Reset to default shader template
- **Load** - Load shader from file
- **Save** - Save shader to file
- **Compile** - Manually recompile (if auto-compile is disabled)
- **Pause** - Pause/resume animation
- **Install to NeoWall** - Install shader to NeoWall directory

**Preview:**
- **Mouse movement** - Updates `iMouse` uniform
- **Pause button** - Freezes animation time

## Project Structure

```
gleditor/
├── src/
│   ├── main.c                  # Application entry point
│   ├── shader_editor.c         # Shader editor implementation
│   ├── shader_editor.h         # Shader editor header
│   ├── common/
│   │   └── log.h              # Logging macros
│   ├── daemon/
│   │   └── command_exec.h     # Stub header (no daemon needed)
│   └── shader_lib/            # Shader compilation library
│       ├── neowall_shader_api.c
│       ├── neowall_shader_api.h
│       ├── shader.h
│       ├── shader_core.c
│       ├── shadertoy_compat.c
│       ├── shadertoy_compat.h
│       ├── shader_adaptation.c
│       └── shader_log.h
├── Makefile                   # Build system
├── README.md                  # This file
└── .gitignore

```

## OpenGL ES Version Detection

The Makefile automatically detects available OpenGL ES versions:

```bash
make info
```

**Output:**
```
============================================
gleditor v1.0.0 - Shader Editor
============================================

OpenGL ES / EGL Support:
  EGL: Yes (version 1.5)
  OpenGL ES 1.x: No
  OpenGL ES 2.0: Yes
  OpenGL ES 3.0: Yes
  OpenGL ES 3.1: Yes
  OpenGL ES 3.2: No

Dependencies:
  GTK+3: Yes
  GTKSourceView 4: Yes
```

## Troubleshooting

### OpenGL ES 2.0 not found

**Error:**
```
OpenGL ES 2.0 not found - minimum requirement
```

**Solution:**
```bash
# Install OpenGL ES development libraries
sudo apt install libgles2-mesa-dev libegl1-mesa-dev
```

### GTK or GTKSourceView not found

**Error:**
```
GTK+3 not found - required for shader editor GUI
```

**Solution:**
```bash
sudo apt install libgtk-3-dev libgtksourceview-4-dev
```

### Shader compilation errors

- Check the **error message** below the editor
- Line numbers are provided for errors
- Ensure your shader follows Shadertoy format
- Use `void mainImage(out vec4 fragColor, in vec2 fragCoord)`

### Preview not updating

- Check if shader compiled successfully (green checkmark)
- Try clicking **Pause** to resume if paused
- Check console output with `--verbose` flag

### Performance issues

- Lower the preview FPS (default: 60 FPS)
- Simplify complex shaders
- Reduce resolution by resizing the window

## Development

### Building with Debug Symbols

```bash
make debug
gdb ./bin/gleditor
```

### Code Style

- **Indentation:** 4 spaces
- **Brace style:** K&R
- **Line length:** 100 characters max
- **Comments:** Clear and concise

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Architecture

### Shader Compilation Pipeline

```
User GLSL Code
     ↓
shader_core.c (detect format)
     ↓
shadertoy_compat.c (wrap if needed)
     ↓
shader_adaptation.c (ES2/ES3 adaptation)
     ↓
neowall_shader_api.c (compile)
     ↓
OpenGL Program
```

### Components

- **shader_editor.c** - GTK UI, OpenGL rendering
- **shader_core.c** - Core shader compilation
- **shadertoy_compat.c** - Shadertoy format wrapper
- **shader_adaptation.c** - ES version adaptation
- **neowall_shader_api.c** - High-level API

## License

MIT License - See LICENSE file for details

## Credits

- **NeoWall Project** - Live wallpaper daemon
- **Shadertoy** - Shader format inspiration
- **GTK** - UI toolkit
- **GTKSourceView** - Code editor component

## Links

- **NeoWall:** [github.com/yourusername/neowall](https://github.com/yourusername/neowall)
- **Shadertoy:** [shadertoy.com](https://www.shadertoy.com)
- **Issue Tracker:** [github.com/yourusername/gleditor/issues](https://github.com/yourusername/gleditor/issues)

## Examples

### Simple Gradient

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = vec4(uv, 0.5 + 0.5 * sin(iTime), 1.0);
}
```

### Animated Circle

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    float d = length(uv);
    float c = smoothstep(0.3, 0.29, d);
    vec3 col = vec3(c) * (0.5 + 0.5 * cos(iTime + vec3(0, 2, 4)));
    fragColor = vec4(col, 1.0);
}
```

### Plasma Effect

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    float t = iTime * 0.5;
    
    float c = sin(uv.x * 10.0 + t);
    c += sin(uv.y * 10.0 + t);
    c += sin((uv.x + uv.y) * 10.0 + t);
    c += sin(length(uv - 0.5) * 20.0 + t);
    
    vec3 col = vec3(0.5, 0.3, 0.8) + 0.5 * cos(c + vec3(0, 1, 2));
    fragColor = vec4(col, 1.0);
}
```

---

**Happy Shader Coding! 🎨✨**