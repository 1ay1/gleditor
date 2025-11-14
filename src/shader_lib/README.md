# Shader Library Documentation

## Overview

The NeoWall Shader Library (`shader_lib/`) is a comprehensive, modular shader compilation system designed to support both the NeoWall daemon and the GTK shader editor. It provides robust shader compilation, Shadertoy compatibility, automatic version adaptation, and detailed error reporting.

## Architecture

```
shader_lib/
├── shader.h                  # Public API for shader operations
├── shader_core.c             # Core compilation and program creation
├── shader_log.h              # Lightweight logging system
├── shader_adaptation.c       # GLSL version adaptation (ES 2.0 ↔ ES 3.0)
├── shadertoy_compat.h/c      # Shadertoy format preprocessing
├── neowall_shader_api.h/c    # High-level unified API wrapper
└── README.md                 # This file
```

## Components

### 1. Core Shader Compilation (`shader_core.c`)

The heart of the shader library, providing:

#### Key Functions

- **`shader_create_program_from_sources()`**
  ```c
  bool shader_create_program_from_sources(const char *vertex_src, 
                                          const char *fragment_src,
                                          GLuint *program);
  ```
  Creates a complete shader program from vertex and fragment source code.
  Handles compilation, linking, and error reporting.

- **`shader_create_live_program()`**
  ```c
  bool shader_create_live_program(const char *shader_path, 
                                  GLuint *program, 
                                  size_t channel_count);
  ```
  High-level function for live wallpaper shaders. Automatically:
  - Loads shader from file
  - Detects Shadertoy format
  - Wraps user code with compatibility layer
  - Resolves includes and dependencies
  - Compiles and links complete program

- **`shader_destroy_program()`**
  ```c
  void shader_destroy_program(GLuint program);
  ```
  Safely destroys a shader program and frees OpenGL resources.

#### Shadertoy Wrapper System

The library automatically wraps Shadertoy-style shaders with a compatibility layer that provides:

**Standard Uniforms:**
```glsl
uniform float _neowall_time;          // Internal time uniform
uniform vec2 _neowall_resolution;     // Internal resolution uniform
uniform vec3 iResolution;             // Shadertoy iResolution
uniform vec4 iChannelTime[4];         // Per-channel time
uniform vec3 iChannelResolution[4];   // Per-channel resolution
```

**Shadertoy-Compatible Globals:**
```glsl
float iTime;                          // Shader playback time (seconds)
float time;                           // Alias for iTime
vec2 resolution;                      // Screen resolution
float iTimeDelta;                     // Time between frames
int iFrame;                           // Frame counter
vec4 iMouse;                          // Mouse coordinates
vec4 iDate;                           // Current date/time
float iSampleRate;                    // Audio sample rate
```

**Texture Channels:**
```glsl
uniform sampler2D iChannel0;          // Texture channel 0
uniform sampler2D iChannel1;          // Texture channel 1
uniform sampler2D iChannel2;          // Texture channel 2
uniform sampler2D iChannel3;          // Texture channel 3
```

#### Shader Detection & Adaptation

The library intelligently detects:
- **Shadertoy Format**: Presence of `mainImage()` function
- **Version Conflicts**: Duplicate uniforms, global variables
- **GLSL Version**: ES 2.0 vs ES 3.0 syntax
- **Include Directives**: `#include "file.glsl"`

### 2. Shadertoy Compatibility (`shadertoy_compat.c`)

Provides preprocessing for Shadertoy shaders:

#### Key Functions

- **`shadertoy_preprocess()`**
  ```c
  char *shadertoy_preprocess(const char *source);
  ```
  Preprocesses Shadertoy shader source:
  - Detects texture channel usage (iChannel0-3)
  - Replaces texture lookups with noise-based fallbacks
  - Handles `textureLod()` and `texelFetch()` calls
  - Returns preprocessed source (caller must free)

- **`shadertoy_convert_texture_calls()`**
  ```c
  char *shadertoy_convert_texture_calls(const char *source);
  ```
  Converts GLSL 3.0 texture functions to GLSL ES 1.0:
  - `texture(sampler, uv)` → `texture2D(sampler, uv)`
  - `textureLod(sampler, uv, lod)` → `texture2D(sampler, uv)`
  - Only converts iChannel samplers, leaves others unchanged

- **`shadertoy_analyze_shader()`**
  ```c
  void shadertoy_analyze_shader(const char *source);
  ```
  Analyzes shader and logs detailed information about:
  - Detected features (textures, mouse input, etc.)
  - Complexity estimation
  - Performance expectations
  - Potential compatibility issues

#### Noise Functions

When texture channels are unavailable, the library provides procedural noise:

```glsl
// Hash function for pseudo-random values
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

// 2D noise function
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
```

### 3. Version Adaptation (`shader_adaptation.c`)

Automatically adapts shaders between OpenGL ES versions:

#### Key Functions

- **`adapt_shader_for_version()`**
  ```c
  char *adapt_shader_for_version(bool use_es3,
                                  const char *shader_code,
                                  bool is_fragment_shader);
  ```
  Main adaptation function that converts shaders between ES 2.0 and ES 3.0.

- **`adapt_vertex_shader()` / `adapt_fragment_shader()`**
  ```c
  char *adapt_vertex_shader(bool use_es3, const char *shader_code);
  char *adapt_fragment_shader(bool use_es3, const char *shader_code);
  ```
  Convenience wrappers for specific shader types.

#### Conversion Tables

**ES 3.0 → ES 2.0:**
- `#version 300 es` → `#version 100`
- `texture()` → `texture2D()`
- `in` → `varying` (fragment) / `attribute` (vertex)
- `out` → `varying` (vertex)
- `out vec4 fragColor` → removed (uses `gl_FragColor`)
- `fragColor` → `gl_FragColor`

**ES 2.0 → ES 3.0:**
- `#version 100` → `#version 300 es`
- `texture2D()` → `texture()`
- `varying` → `in` (fragment) / `out` (vertex)
- `attribute` → `in` (vertex)
- `gl_FragColor` → `fragColor`
- Adds `out vec4 fragColor;` declaration

### 4. Unified API (`neowall_shader_api.c`)

High-level API wrapper for easy integration:

#### Data Structures

```c
typedef struct {
    GLuint program;           // Compiled shader program (0 if failed)
    bool success;             // Whether compilation succeeded
    char *error_message;      // Error message if failed (must be freed)
    int error_line;           // Line number where error occurred
} neowall_shader_result_t;

typedef struct {
    bool use_es3;             // Use OpenGL ES 3.0 (true) or 2.0 (false)
    int channel_count;        // Number of texture channels (0-4)
    bool verbose_errors;      // Include full shader source in errors
} neowall_shader_options_t;
```

#### Key Functions

- **`neowall_shader_compile()`**
  ```c
  neowall_shader_result_t neowall_shader_compile(
      const char *shader_source,
      const neowall_shader_options_t *options);
  ```
  Compiles shader from source string.

- **`neowall_shader_compile_file()`**
  ```c
  neowall_shader_result_t neowall_shader_compile_file(
      const char *shader_path,
      const neowall_shader_options_t *options);
  ```
  Compiles shader from file path.

- **`neowall_shader_set_uniforms()`**
  ```c
  void neowall_shader_set_uniforms(GLuint program, 
                                   int width, 
                                   int height, 
                                   float time);
  ```
  Sets standard uniforms for rendering.

- **`neowall_shader_destroy()` / `neowall_shader_free_result()`**
  Resource cleanup functions.

### 5. Logging System (`shader_log.h`)

Lightweight logging that works standalone without daemon dependencies:

```c
#define log_error(...)  // Red error messages
#define log_warn(...)   // Yellow warnings
#define log_info(...)   // Normal info messages
#define log_debug(...)  // Debug messages (verbose)
```

Configurable via `SHADER_LIB_LOG_LEVEL`:
- `LOG_LEVEL_ERROR` (0)
- `LOG_LEVEL_WARN` (1)
- `LOG_LEVEL_INFO` (2)
- `LOG_LEVEL_DEBUG` (3)

## Usage Examples

### Basic Compilation

```c
#include "shader_lib/neowall_shader_api.h"

// Set up options
neowall_shader_options_t options = NEOWALL_SHADER_OPTIONS_DEFAULT;
options.channel_count = 4;

// Compile from file
neowall_shader_result_t result = neowall_shader_compile_file(
    "/path/to/shader.glsl", 
    &options
);

if (result.success) {
    printf("Shader compiled successfully! Program ID: %u\n", result.program);
    
    // Use the shader...
    glUseProgram(result.program);
    neowall_shader_set_uniforms(result.program, 1920, 1080, 0.0f);
    
    // Render...
    
    // Cleanup
    neowall_shader_destroy(result.program);
} else {
    fprintf(stderr, "Compilation failed: %s\n", result.error_message);
    neowall_shader_free_result(&result);
}
```

### Compiling from Source String

```c
const char *shader_source = 
    "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
    "    vec2 uv = fragCoord / iResolution.xy;\n"
    "    fragColor = vec4(uv, 0.5 + 0.5 * sin(iTime), 1.0);\n"
    "}\n";

neowall_shader_result_t result = neowall_shader_compile(
    shader_source, 
    NULL  // Use default options
);

if (result.success) {
    // Use shader...
} else {
    fprintf(stderr, "Error: %s\n", result.error_message);
    neowall_shader_free_result(&result);
}
```

### Low-Level API

```c
#include "shader_lib/shader.h"

const char *vertex_src = 
    "#version 100\n"
    "attribute vec2 position;\n"
    "void main() { gl_Position = vec4(position, 0.0, 1.0); }\n";

const char *fragment_src = 
    "#version 100\n"
    "precision mediump float;\n"
    "void main() { gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";

GLuint program;
if (shader_create_program_from_sources(vertex_src, fragment_src, &program)) {
    printf("Program created: %u\n", program);
    shader_destroy_program(program);
}
```

## Shader Format Requirements

### Shadertoy Format

The library automatically detects and wraps Shadertoy shaders:

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = vec4(uv, 0.5, 1.0);
}
```

### Raw Fragment Shader

Or provide a complete fragment shader:

```glsl
#version 100
precision mediump float;

uniform float _neowall_time;
uniform vec2 _neowall_resolution;

void main() {
    vec2 uv = gl_FragCoord.xy / _neowall_resolution;
    gl_FragColor = vec4(uv, 0.5, 1.0);
}
```

## Error Handling

The library provides detailed error reporting:

### Compilation Errors

```
[2024-01-15 14:30:22] [ShaderLib] [ERROR] Fragment shader compilation failed:
0:42: error: 'undeclared_variable' : undeclared identifier
0:42: error: '=' : cannot convert from 'const mediump float' to 'temp mediump float'
```

### Linking Errors

```
[2024-01-15 14:30:22] [ShaderLib] [ERROR] Program linking failed:
Vertex shader is not compiled
```

### Debug Information

Enable debug logging for detailed information:

```c
// In your code before including shader headers:
#define SHADER_LIB_LOG_LEVEL LOG_LEVEL_DEBUG
#include "shader_lib/shader.h"
```

Output:
```
[2024-01-15 14:30:22] [ShaderLib] [DEBUG] Shader adaptation: shader_version=300, target_es3=0, is_fragment=1
[2024-01-15 14:30:22] [ShaderLib] [INFO] Converting ES 3.0 shader to ES 2.0 for compatibility
[2024-01-15 14:30:22] [ShaderLib] [DEBUG] Detected Shadertoy format (has mainImage function)
[2024-01-15 14:30:22] [ShaderLib] [DEBUG] Building Shadertoy wrapper with 4 channels
[2024-01-15 14:30:22] [ShaderLib] [DEBUG] Vertex shader compiled successfully
[2024-01-15 14:30:22] [ShaderLib] [DEBUG] Fragment shader compiled successfully
[2024-01-15 14:30:22] [ShaderLib] [DEBUG] Shader program created successfully (ID: 42)
```

## Performance Considerations

### Compilation Caching

The library doesn't cache compiled shaders internally. Applications should:
- Store program IDs for reuse
- Avoid recompiling shaders every frame
- Implement their own caching if needed

### Memory Management

All returned strings must be freed by the caller:
```c
char *adapted = adapt_fragment_shader(true, source);
// Use adapted...
free(adapted);

neowall_shader_result_t result = neowall_shader_compile(source, NULL);
// Use result...
neowall_shader_free_result(&result);
```

### Shader Complexity

The Shadertoy analyzer estimates complexity:
- **Low**: Simple gradients, basic math
- **Medium**: Procedural patterns, simple raymarching
- **High**: Complex raymarching, multiple texture lookups
- **Very High**: Advanced effects, nested loops

## Feature Flags

The library respects OpenGL ES version detection:

```c
#ifdef HAVE_GLES3
    // OpenGL ES 3.0 features available
#endif

#ifdef HAVE_GLES31
    // OpenGL ES 3.1 features available
#endif

#ifdef HAVE_GLES32
    // OpenGL ES 3.2 features available
#endif
```

## Testing

### Manual Testing

```bash
# Build with debug symbols
make debug

# Run with verbose logging
SHADER_LIB_LOG_LEVEL=3 ./bin/gleditor
```

### Common Test Cases

1. **Shadertoy Format**: Test with various Shadertoy shaders
2. **Version Adaptation**: Test ES 2.0 and ES 3.0 shaders
3. **Error Handling**: Test with intentionally broken shaders
4. **Includes**: Test `#include` directive resolution
5. **Texture Channels**: Test with/without texture channels

## Limitations

1. **Texture Channels**: Limited to 4 channels (iChannel0-3)
2. **Buffer Passes**: Multi-pass rendering not supported
3. **Cubemaps**: Cubemap textures replaced with procedural noise
4. **Audio**: iChannelN audio input not supported
5. **Mouse Input**: iMouse is always vec4(0.0)
6. **Keyboard**: Keyboard input not available

## Future Enhancements

- [ ] Multi-pass rendering support
- [ ] Real texture loading for channels
- [ ] Mouse/keyboard input support
- [ ] Shader caching system
- [ ] Performance profiling
- [ ] SPIR-V compilation
- [ ] Compute shader support (ES 3.1+)
- [ ] Geometry/tessellation shaders (ES 3.2+)

## Contributing

When modifying the shader library:

1. **Maintain Compatibility**: Don't break existing API
2. **Test Thoroughly**: Test with ES 2.0 and ES 3.0
3. **Document Changes**: Update this README
4. **Log Appropriately**: Use proper log levels
5. **Free Memory**: Always free allocated strings
6. **Check Errors**: Validate all OpenGL operations

## References

- [Shadertoy](https://www.shadertoy.com/) - Shader examples and format
- [OpenGL ES 2.0 Spec](https://www.khronos.org/opengles/2_X/)
- [OpenGL ES 3.0 Spec](https://www.khronos.org/opengles/3_X/)
- [GLSL ES Spec](https://www.khronos.org/registry/OpenGL/specs/es/3.0/GLSL_ES_Specification_3.00.pdf)

## License

Part of the NeoWall project. See main project LICENSE file.