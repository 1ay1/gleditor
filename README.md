# gleditor 🎨✨

**The Shader Editor That Actually Works™**

A standalone OpenGL ES shader editor that doesn't make you want to flip your desk. Create, test, and install shaders for NeoWall without sacrificing your sanity.

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Sanity](https://img.shields.io/badge/developer_sanity-barely_intact-orange)

## Why gleditor?

Because editing shaders in Vim while your wallpaper crashes in the background is a form of masochism we're trying to eliminate. Also, we needed a catchy name.

## Features

- 🎨 **Live Preview** - See your shader in action before your GPU melts
- 📝 **Syntax Highlighting** - Because reading plain text GLSL is like reading the Matrix
- 🔧 **OpenGL ES Support** - We support ES 2.0, 3.0, 3.1, and 3.2 (yes, even the weird ones)
- 🎯 **Shadertoy Compatible** - Copy-paste from Shadertoy and it just works* (*most of the time)
- 💾 **Save/Load** - Revolutionary concept: actually save your work
- 📦 **NeoWall Integration** - One-click install to NeoWall (no command line sorcery required)
- ⚡ **Auto-Compile** - Types as you type. It's like autocomplete but more judgy
- 🖱️ **Mouse Tracking** - Your shader can watch you work. Creepy? Maybe. Cool? Definitely.
- 🟢 **Matrix Green FPS Counter** - Because everything looks more professional in #00FF41

## Screenshots

```
┌─────────────────────────────────────────────────────────────────┐
│ 🎨 gleditor                                            [_][□][X] │
├─────────────────────────────────────────────────────────────────┤
│ [🆕 New] [📂 Load] [💾 Save] │ [⚡ Compile] [⏸ Pause] │ [📦 Install to NeoWall] │    ⚡ FPS: 60   │
├──────────────────────────┬──────────────────────────────────────┤
│ 📝 Shader Code          │  👁️  Live Preview                    │
│ ┌────────────────────┐  │  ┌──────────────────────────────┐   │
│ │ void mainImage(... │  │  │                              │   │
│ │ {                  │  │  │   [Rainbow vomit intensifies] │   │
│ │   vec2 uv = ...    │  │  │                              │   │
│ │   vec3 col = 0.5 + │  │  │   Your shader goes here!     │   │
│ │     0.5 * cos(...  │  │  │                              │   │
│ │   fragColor = ...  │  │  │   (hopefully without crashing)│   │
│ │ }                  │  │  │                              │   │
│ └────────────────────┘  │  └──────────────────────────────┘   │
│ ✓ Compiled successfully │                                      │
├──────────────────────────┴──────────────────────────────────────┤
│ ✓ Ready to break things                      📍 Line 1, Col 1 │
└─────────────────────────────────────────────────────────────────┘
```

*Actual interface is less ASCII and more GTK. We promise it's prettier.*

## Requirements

### Build Dependencies

You'll need these before the compiler yells at you:

- **GTK+3** (>= 3.20) - Because X11 is so 1990s
- **GTKSourceView 4** - For that sweet syntax highlighting
- **OpenGL ES 2.0** (minimum) - The ancient texts require this
- **EGL** - For talking to your GPU without crying
- **GCC** - Or any C compiler that doesn't judge you
- **pkg-config** - The unsung hero of Linux development

### Optional (but recommended if you want the fancy stuff)

- OpenGL ES 3.0+ - For shaders that don't look like they're from 2005
- OpenGL ES 3.1 - Compute shaders! (for when regular shaders aren't painful enough)
- OpenGL ES 3.2 - Geometry and tessellation shaders (show-offs welcome)

## Installation

### The "I Know What I'm Doing" Route

```bash
git clone https://github.com/yourusername/gleditor.git
cd gleditor
make
./bin/gleditor  # 🎉
```

### The "Please Hold My Hand" Route

**Step 1:** Install dependencies (aka "Feeding the Beast")

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
sudo pacman -S base-devel gtk3 gtksourceview4 mesa glu
```

*Note: If you use Arch, you probably already know this. You also probably told us.*

**Step 2:** Build it (the exciting part)

```bash
make
```

If it works, you'll see a bunch of satisfying green text. If it doesn't, welcome to C development! Check the error messages (they're trying to help, we promise).

**Step 3:** Run it

```bash
./bin/gleditor
```

**Step 4:** Install it system-wide (optional, for the brave)

```bash
sudo make install
```

Now you can just type `gleditor` from anywhere, like a boss.

### Build Options (for the tinkerers)

```bash
make          # Build the thing
make run      # Build and launch (one command to rule them all)
make clean    # Remove build artifacts (fresh start therapy)
make install  # Install to /usr/local/bin
make debug    # Build with debug symbols (for when things go wrong)
make info     # Display OpenGL ES detection info (nerd stats)
make help     # Show all available commands (RTFM as a service)
```

## Usage

### Starting the Editor

```bash
gleditor [OPTIONS]
```

**Options:**
- `-v, --version` - Show version and OpenGL ES support (flex on your friends)
- `-V, --verbose` - Enable verbose output (for debugging or feeling important)
- `-h, --help` - Show help message (no shame in asking)

### Creating a Shader (The Fun Part)

1. **Start with the default template** - It's a rainbow gradient. Very web 2.0.
2. **Edit the shader** in the left pane - Try not to divide by zero
3. **Watch the live preview** in the right pane - If it crashes, it's a feature
4. Shaders auto-compile as you type (500ms debounce, because we're not monsters)

### Shader Format (Important Stuff)

gleditor uses **Shadertoy-compatible** format. This means you can steal... err, *borrow* shaders from Shadertoy:

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Your artistic vision goes here
    vec2 uv = fragCoord / iResolution.xy;
    
    // Make it move (because static wallpapers are boring)
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));
    
    // Output to screen (the moment of truth)
    fragColor = vec4(col, 1.0);
}
```

### Available Uniforms (The Magic Variables)

These are provided automatically, like a shader fairy godmother:

- `vec3 iResolution` - Viewport resolution (width, height, aspect ratio)
- `float iTime` - Shader playback time in seconds (keeps counting forever)
- `float iTimeDelta` - Time since last frame (for the physics nerds)
- `int iFrame` - Frame number (in case you're counting)
- `vec4 iMouse` - Mouse position (x, y, click_x, click_y)

### Saving and Loading (Don't Lose Your Work!)

- **💾 Save** - Saves your shader to a `.glsl` file
- **📂 Load** - Loads a shader from disk
- **🆕 New** - Reset to default template (with confirmation, we're not savages)

⚠️ **Important:** No autosave! Save your work or weep later.

### Installing to NeoWall (The Whole Point)

1. Click **📦 Install to NeoWall**
2. Enter a shader name (be creative, but also descriptive)
3. Your shader magically appears in `~/.config/neowall/shaders/`
4. Configure NeoWall to use your shader
5. Enjoy your animated wallpaper
6. Tell everyone you made it

### Controls (Buttons That Do Things)

**Toolbar:**
- **🆕 New** - Fresh start (goodbye mistakes)
- **📂 Load** - Load a shader file (remember where you saved it)
- **💾 Save** - Save your masterpiece (do this often)
- **⚡ Compile** - Manually recompile (for when auto-compile is too slow)
- **⏸ Pause** - Freeze the animation (for screenshots or debugging)
- **📦 Install to NeoWall** - One-click deployment (feel like a DevOps engineer)

**Preview Interactions:**
- **Mouse movement** - Updates `iMouse` uniform (make interactive shaders!)
- **Pause button** - Stops time (eat your heart out, Doctor Who)

## Project Structure (For the Curious)

```
gleditor/
├── src/
│   ├── main.c                  # Application entry point (the beginning)
│   ├── shader_editor.c         # Where the magic happens (5000+ lines of pain)
│   ├── shader_editor.h         # Header file (because C requires it)
│   └── shader_lib/            # Shader compilation library (the serious stuff)
│       ├── neowall_shader_api.c    # High-level API
│       ├── neowall_shader_api.h
│       ├── shader.h                # Core shader definitions
│       ├── shader_core.c           # Compilation engine
│       ├── shadertoy_compat.c      # Shadertoy wrapper
│       ├── shadertoy_compat.h
│       ├── shader_adaptation.c     # ES version adaptation
│       └── shader_log.h            # Logging utilities
├── Makefile                   # Build system (reads your mind)
├── README.md                  # You are here
└── .gitignore                 # Files we pretend don't exist
```

## OpenGL ES Version Detection (Nerd Section)

The Makefile automatically detects what OpenGL ES versions you have:

```bash
make info
```

**Sample Output:**
```
============================================
gleditor v1.0.0 - Shader Editor
============================================

OpenGL ES / EGL Support:
  EGL: Yes (version 1.5)
  OpenGL ES 1.x: No (and you don't need it)
  OpenGL ES 2.0: Yes (the minimum requirement)
  OpenGL ES 3.0: Yes (getting fancy!)
  OpenGL ES 3.1: Yes (compute shaders unlocked)
  OpenGL ES 3.2: No (maybe next GPU)

Dependencies:
  GTK+3: Yes
  GTKSourceView 4: Yes

Build Configuration:
  CC: gcc
  PREFIX: /usr/local
```

## Troubleshooting (When Things Go Wrong)

### "OpenGL ES 2.0 not found"

**Translation:** Your system is missing OpenGL ES libraries.

**Fix:**
```bash
# Debian/Ubuntu
sudo apt install libgles2-mesa-dev libegl1-mesa-dev

# Fedora
sudo dnf install mesa-libGLES-devel mesa-libEGL-devel

# Arch
sudo pacman -S mesa
```

### "GTK or GTKSourceView not found"

**Translation:** Missing GUI libraries.

**Fix:**
```bash
# Debian/Ubuntu
sudo apt install libgtk-3-dev libgtksourceview-4-dev

# Fedora
sudo dnf install gtk3-devel gtksourceview4-devel

# Arch
sudo pacman -S gtk3 gtksourceview4
```

### "Shader compilation errors"

**Possible causes:**
1. You divided by zero (classic mistake)
2. Typo in variable name (GLSL is case-sensitive)
3. Missing semicolon (yes, in 2025)
4. Forgot to declare a variable (shader compiler is strict)

**What to do:**
- Check the error message below the editor (it has line numbers!)
- Make sure your shader follows Shadertoy format
- Use `void mainImage(out vec4 fragColor, in vec2 fragCoord)`
- Remember: GLSL is not JavaScript. Types matter.

### "Preview not updating"

**Checklist:**
- [ ] Did the shader compile? (check for green checkmark)
- [ ] Is the animation paused? (click the pause button)
- [ ] Is your GPU on fire? (check temperature)

### "Performance issues / Low FPS"

**Solutions:**
- Simplify your shader (fewer operations = happier GPU)
- Reduce preview window size (less pixels = faster rendering)
- Lower iteration counts in loops (nobody needs 1000 iterations)
- Check if your shader has infinite loops (narrator: it probably does)

## Development (Join the Chaos)

### Building with Debug Symbols

```bash
make debug
gdb ./bin/gleditor
```

### Code Style (Guidelines We Sometimes Follow)

- **Indentation:** 4 spaces (tabs are a lie)
- **Brace style:** K&R (the one true style)
- **Line length:** 100 characters max (readability matters)
- **Comments:** More is better (your future self will thank you)
- **Variable names:** Descriptive (no `int x;` unless it's coordinates)

### Contributing

1. Fork the repository (button in top right)
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes (try not to break everything)
4. Test thoroughly (yes, actually test it)
5. Commit with descriptive messages (no "fixed stuff")
6. Push to your fork (`git push origin feature/amazing-feature`)
7. Open a Pull Request (describe what you did and why)

## Architecture (For the Deeply Curious)

### Shader Compilation Pipeline

```
User GLSL Code
     ↓
shader_core.c (detect format - is it Shadertoy?)
     ↓
shadertoy_compat.c (wrap if needed - add uniforms and boilerplate)
     ↓
shader_adaptation.c (adapt for ES2/ES3 - version compatibility)
     ↓
neowall_shader_api.c (compile - pray to the GPU gods)
     ↓
OpenGL Program (success!) or Error Messages (sadness)
```

### Components (What Does What)

- **shader_editor.c** - GTK UI and OpenGL rendering (the frontend)
- **shader_core.c** - Core shader compilation logic (the backend)
- **shadertoy_compat.c** - Wraps Shadertoy format (the adapter)
- **shader_adaptation.c** - ES version compatibility (the diplomat)
- **neowall_shader_api.c** - High-level API (the interface)

## Keyboard Shortcuts (For the Efficiency Addicts)

*Coming soon! Or use the mouse, we won't judge.*

## Fun Facts

- This editor was built because editing shaders in Nano was driving someone insane
- The matrix green theme is mandatory (it's not a phase, mom)
- The FPS counter updates 60 times per second (very meta)
- Auto-compile has a 500ms debounce (we value your CPU)
- The preview starts with a rainbow gradient (because why not?)

## License

MIT License - Do whatever you want with this code. Break it, fix it, make it better. We believe in you!

## Credits

- **NeoWall Project** - The reason this exists
- **Shadertoy** - For the shader format and inspiration
- **GTK** - For making GUIs possible without crying
- **GTKSourceView** - For syntax highlighting magic
- **Coffee** - For making this project possible
- **You** - For actually reading this README

## Links

- **NeoWall:** [github.com/yourusername/neowall](https://github.com/yourusername/neowall)
- **Shadertoy:** [shadertoy.com](https://www.shadertoy.com)
- **Issue Tracker:** [github.com/yourusername/gleditor/issues](https://github.com/yourusername/gleditor/issues)
- **Bug Reports:** Same as above (we know there are bugs)

## Example Shaders (To Get You Started)

### Simple Gradient (Baby's First Shader)

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = vec4(uv, 0.5 + 0.5 * sin(iTime), 1.0);
}
```

### Animated Circle (Getting Fancy)

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Center and normalize coordinates
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
    // Distance from center
    float d = length(uv);
    
    // Animated circle with smooth edges
    float c = smoothstep(0.3, 0.29, d);
    
    // Color based on time
    vec3 col = vec3(c) * (0.5 + 0.5 * cos(iTime + vec3(0, 2, 4)));
    
    fragColor = vec4(col, 1.0);
}
```

### Plasma Effect (Now We're Talking)

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    float t = iTime * 0.5;
    
    // Multiple sine waves create plasma effect
    float c = sin(uv.x * 10.0 + t);
    c += sin(uv.y * 10.0 + t);
    c += sin((uv.x + uv.y) * 10.0 + t);
    c += sin(length(uv - 0.5) * 20.0 + t);
    
    // Color mapping
    vec3 col = vec3(0.5, 0.3, 0.8) + 0.5 * cos(c + vec3(0, 1, 2));
    fragColor = vec4(col, 1.0);
}
```

### Starfield (For the Space Enthusiasts)

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    uv -= 0.5;  // Center
    uv.x *= iResolution.x / iResolution.y;  // Aspect ratio correction
    
    float stars = 0.0;
    for (float i = 0.0; i < 50.0; i++) {
        vec2 pos = vec2(
            sin(i * 12.345) * 0.5,
            cos(i * 45.678) * 0.5
        );
        
        float dist = length(uv - pos);
        stars += 0.001 / (dist * dist);
    }
    
    vec3 col = vec3(stars);
    fragColor = vec4(col, 1.0);
}
```

## FAQ (Frequently Asked Questions)

**Q: Why "gleditor"?**  
A: Because "OpenGL ES Shader Editor" was too long, and "ShaderThing" was taken.

**Q: Will this work on Windows?**  
A: Probably not out of the box. But WSL exists, and that's close enough!

**Q: Can I use this for game development?**  
A: Sure! It's designed for wallpapers, but shaders are shaders.

**Q: Why matrix green everywhere?**  
A: Because we can. Also, it looks cool at 2 AM.

**Q: My shader makes the preview window go black. Is this broken?**  
A: No, your shader is probably broken. Check for compilation errors.

**Q: Can I contribute?**  
A: Please do! We need all the help we can get.

**Q: Is there a dark mode?**  
A: Everything is dark mode if you close your eyes.

---

**Happy Shader Coding! May your gradients be smooth and your FPS be high.** 🎨✨🚀

*Built with ❤️ and way too much caffeine*