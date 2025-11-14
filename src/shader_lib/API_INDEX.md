# Shader Library API Index

Complete reference for all shader library functions, types, and macros.

## Table of Contents

- [High-Level API](#high-level-api)
- [Low-Level API](#low-level-api)
- [Utility Functions](#utility-functions)
- [Shadertoy Compatibility](#shadertoy-compatibility)
- [Version Adaptation](#version-adaptation)
- [Data Types](#data-types)
- [Logging](#logging)

---

## High-Level API

**Header:** `neowall_shader_api.h`

### Functions

#### `neowall_shader_compile()`
```c
neowall_shader_result_t neowall_shader_compile(
    const char *shader_source,
    const neowall_shader_options_t *options
);
```
Compile a shader from source code string.

**Parameters:**
- `shader_source`: Shader source code (Shadertoy or raw GLSL)
- `options`: Compilation options (NULL for defaults)

**Returns:** `neowall_shader_result_t` with program ID and status

**Example:**
```c
neowall_shader_result_t result = neowall_shader_compile(source, NULL);
if (result.success) {
    glUseProgram(result.program);
}
```

---

#### `neowall_shader_compile_file()`
```c
neowall_shader_result_t neowall_shader_compile_file(
    const char *shader_path,
    const neowall_shader_options_t *options
);
```
Compile a shader from file path.

**Parameters:**
- `shader_path`: Path to shader file
- `options`: Compilation options (NULL for defaults)

**Returns:** `neowall_shader_result_t` with program ID and status

---

#### `neowall_shader_destroy()`
```c
void neowall_shader_destroy(GLuint program);
```
Destroy a compiled shader program.

**Parameters:**
- `program`: OpenGL program ID to destroy

---

#### `neowall_shader_free_result()`
```c
void neowall_shader_free_result(neowall_shader_result_t *result);
```
Free error message from shader result.

**Parameters:**
- `result`: Pointer to result structure

---

#### `neowall_shader_set_uniforms()`
```c
void neowall_shader_set_uniforms(
    GLuint program,
    int width,
    int height,
    float time
);
```
Set standard NeoWall uniforms for rendering.

**Parameters:**
- `program`: Shader program ID
- `width`: Viewport width in pixels
- `height`: Viewport height in pixels
- `time`: Current time in seconds

**Sets:**
- `_neowall_time` (float)
- `_neowall_resolution` (vec2)
- `iResolution` (vec3)

---

#### `neowall_shader_get_vertex_source()`
```c
const char *neowall_shader_get_vertex_source(bool use_es3);
```
Get vertex shader source for fullscreen quad.

**Parameters:**
- `use_es3`: Use ES 3.0 syntax (true) or ES 2.0 (false)

**Returns:** Static string (do not free)

---

## Low-Level API

**Header:** `shader.h`

### Functions

#### `shader_create_program_from_sources()`
```c
bool shader_create_program_from_sources(
    const char *vertex_src,
    const char *fragment_src,
    GLuint *program
);
```
Create shader program from vertex and fragment sources.

**Parameters:**
- `vertex_src`: Vertex shader source code
- `fragment_src`: Fragment shader source code
- `program`: Output pointer for program ID

**Returns:** `true` on success, `false` on failure

---

#### `shader_create_live_program()`
```c
bool shader_create_live_program(
    const char *shader_path,
    GLuint *program,
    size_t channel_count
);
```
Create live wallpaper shader program from file.

**Parameters:**
- `shader_path`: Path to fragment shader file
- `program`: Output pointer for program ID
- `channel_count`: Number of texture channels (0 = default 4)

**Returns:** `true` on success, `false` on failure

**Features:**
- Automatic Shadertoy detection
- Wrapper generation
- Include resolution
- Version adaptation

---

#### `shader_destroy_program()`
```c
void shader_destroy_program(GLuint program);
```
Destroy shader program (same as high-level version).

---

## Utility Functions

**Header:** `shader_utils.h`

### Analysis Functions

#### `shader_parse_error_log()`
```c
shader_error_info_t *shader_parse_error_log(
    const char *shader_log,
    const char *shader_source
);
```
Parse OpenGL error log and extract line numbers.

**Returns:** `shader_error_info_t*` (caller must free)

---

#### `shader_get_statistics()`
```c
shader_stats_t *shader_get_statistics(const char *shader_source);
```
Analyze shader and return statistics.

**Returns:** `shader_stats_t*` (caller must free)

---

#### `shader_validate_syntax()`
```c
shader_validation_t *shader_validate_syntax(
    const char *shader_source,
    bool is_fragment
);
```
Validate shader syntax without compiling.

**Parameters:**
- `shader_source`: Shader code
- `is_fragment`: true for fragment, false for vertex

**Returns:** `shader_validation_t*` (caller must free)

---

### Formatting Functions

#### `shader_format_source()`
```c
char *shader_format_source(const char *shader_source);
```
Format shader with proper indentation.

**Returns:** Formatted string (caller must free)

---

#### `shader_add_line_numbers()`
```c
char *shader_add_line_numbers(
    const char *shader_source,
    int start_line
);
```
Add line numbers to shader source.

**Parameters:**
- `shader_source`: Shader code
- `start_line`: Starting line number (usually 1)

**Returns:** Numbered string (caller must free)

---

#### `shader_strip_comments()`
```c
char *shader_strip_comments(
    const char *shader_source,
    bool keep_newlines
);
```
Remove comments from shader.

**Parameters:**
- `shader_source`: Shader code
- `keep_newlines`: Preserve line breaks

**Returns:** Stripped string (caller must free)

---

### Template Functions

#### `shader_get_template()`
```c
const char *shader_get_template(const char *template_name);
```
Get shader template by name.

**Available templates:**
- `"basic"` - Simple gradient
- `"animated"` - Time-based animation
- `"plasma"` - Plasma effect
- `"noise"` - Noise pattern
- `"raymarch"` - Raymarching template
- `"shadertoy"` - Shadertoy boilerplate

**Returns:** Static string (do not free)

---

#### `shader_list_templates()`
```c
const char **shader_list_templates(size_t *count);
```
List all available template names.

**Parameters:**
- `count`: Output parameter for number of templates

**Returns:** Static array (do not free)

---

### Extraction Functions

#### `shader_extract_uniforms()`
```c
size_t shader_extract_uniforms(
    const char *shader_source,
    char ***uniform_names,
    char ***uniform_types
);
```
Extract uniform declarations.

**Parameters:**
- `shader_source`: Shader code
- `uniform_names`: Output array of names
- `uniform_types`: Output array of types

**Returns:** Number of uniforms found

**Cleanup:** Use `shader_free_uniforms()`

---

#### `shader_extract_functions()`
```c
size_t shader_extract_functions(
    const char *shader_source,
    char ***function_names,
    char ***function_signatures
);
```
Extract function signatures.

**Returns:** Number of functions found

**Cleanup:** Use `shader_free_functions()`

---

### Performance Functions

#### `shader_estimate_performance()`
```c
int shader_estimate_performance(const char *shader_source);
```
Estimate shader performance (0=best, 100=worst).

**Returns:** Performance score

---

#### `shader_get_performance_tips()`
```c
size_t shader_get_performance_tips(
    const char *shader_source,
    char ***recommendations
);
```
Get optimization recommendations.

**Returns:** Number of recommendations

**Cleanup:** Use `shader_free_recommendations()`

---

### Code Generation

#### `shader_generate_fullscreen_vertex()`
```c
const char *shader_generate_fullscreen_vertex(bool use_es3);
```
Generate vertex shader for fullscreen quad.

**Returns:** Static string (do not free)

---

#### `shader_generate_fragment_boilerplate()`
```c
char *shader_generate_fragment_boilerplate(
    bool use_es3,
    bool include_time,
    bool include_resolution
);
```
Generate fragment shader boilerplate.

**Returns:** Generated code (caller must free)

---

### Miscellaneous

#### `shader_minify()`
```c
char *shader_minify(const char *shader_source);
```
Minify shader (remove whitespace/comments).

**Returns:** Minified string (caller must free)

---

#### `shader_estimate_size()`
```c
size_t shader_estimate_size(const char *shader_source);
```
Calculate estimated size after minification.

**Returns:** Size in bytes

---

#### `shader_is_likely_valid()`
```c
bool shader_is_likely_valid(const char *shader_source);
```
Quick sanity check for valid GLSL.

**Returns:** `true` if appears valid

---

#### `shader_detect_version()`
```c
int shader_detect_version(const char *shader_source);
```
Detect GLSL version from source.

**Returns:** Version number (100, 300, etc.) or 0

---

#### `shader_generate_description()`
```c
char *shader_generate_description(const char *shader_source);
```
Generate human-readable description.

**Returns:** Description string (caller must free)

---

## Shadertoy Compatibility

**Header:** `shadertoy_compat.h`

### Functions

#### `shadertoy_preprocess()`
```c
char *shadertoy_preprocess(const char *source);
```
Preprocess Shadertoy shader for compatibility.

**Features:**
- Detects texture channel usage
- Replaces texture lookups with noise fallbacks
- Handles `textureLod()` and `texelFetch()`

**Returns:** Preprocessed string (caller must free)

---

#### `shadertoy_convert_texture_calls()`
```c
char *shadertoy_convert_texture_calls(const char *source);
```
Convert GLSL 3.0 `texture()` to GLSL ES 1.0 `texture2D()`.

**Returns:** Converted string (caller must free)

---

#### `shadertoy_analyze_shader()`
```c
void shadertoy_analyze_shader(const char *source);
```
Analyze shader and log information about features.

---

## Version Adaptation

**Header:** `shader_adaptation.c` (internal)

### Functions

#### `adapt_shader_for_version()`
```c
char *adapt_shader_for_version(
    bool use_es3,
    const char *shader_code,
    bool is_fragment_shader
);
```
Adapt shader between ES 2.0 and ES 3.0.

**Returns:** Adapted string (caller must free)

---

#### `adapt_vertex_shader()`
```c
char *adapt_vertex_shader(bool use_es3, const char *shader_code);
```
Adapt vertex shader.

---

#### `adapt_fragment_shader()`
```c
char *adapt_fragment_shader(bool use_es3, const char *shader_code);
```
Adapt fragment shader.

---

## Data Types

### `neowall_shader_result_t`
```c
typedef struct {
    GLuint program;         // Compiled program (0 if failed)
    bool success;           // Compilation success
    char *error_message;    // Error message (must be freed)
    int error_line;         // Error line number (-1 if unknown)
} neowall_shader_result_t;
```

### `neowall_shader_options_t`
```c
typedef struct {
    bool use_es3;           // Use OpenGL ES 3.0
    int channel_count;      // Number of texture channels (0-4)
    bool verbose_errors;    // Include full source in errors
} neowall_shader_options_t;
```

**Default:**
```c
#define NEOWALL_SHADER_OPTIONS_DEFAULT { \
    .use_es3 = false, \
    .channel_count = 4, \
    .verbose_errors = false \
}
```

### `shader_error_info_t`
```c
typedef struct {
    int line_number;        // Error line (-1 if unknown)
    char *message;          // Error message (must be freed)
    char *code_snippet;     // Code snippet (must be freed)
} shader_error_info_t;
```

### `shader_stats_t`
```c
typedef struct {
    size_t line_count;
    size_t uniform_count;
    size_t texture_count;
    size_t function_count;
    bool uses_loops;
    bool uses_conditionals;
    bool is_shadertoy_format;
    int complexity_score;   // 0-100
} shader_stats_t;
```

### `shader_validation_t`
```c
typedef struct {
    bool is_valid;
    bool has_main;
    bool has_version;
    int detected_version;
    char **warnings;
    size_t warning_count;
    char **errors;
    size_t error_count;
} shader_validation_t;
```

---

## Logging

**Header:** `shader_log.h`

### Macros

```c
#define log_error(...)   // Error messages (always shown)
#define log_warn(...)    // Warnings
#define log_info(...)    // Info messages
#define log_debug(...)   // Debug messages (verbose)
```

### Log Levels

```c
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3
```

**Set level:**
```c
#define SHADER_LIB_LOG_LEVEL LOG_LEVEL_DEBUG
```

---

## Standard Shader Uniforms

When using Shadertoy format, these uniforms are automatically available:

### Time & Animation
- `float iTime` - Shader playback time (seconds)
- `float time` - Alias for iTime
- `float iTimeDelta` - Time between frames (~0.016667)
- `int iFrame` - Frame counter

### Resolution & Screen
- `vec3 iResolution` - Viewport resolution (width, height, aspect)
- `vec2 resolution` - Alias for iResolution.xy

### Input
- `vec4 iMouse` - Mouse coordinates (x, y, click_x, click_y)

### Textures
- `sampler2D iChannel0` - Texture channel 0
- `sampler2D iChannel1` - Texture channel 1
- `sampler2D iChannel2` - Texture channel 2
- `sampler2D iChannel3` - Texture channel 3
- `vec4 iChannelTime[4]` - Per-channel time
- `vec3 iChannelResolution[4]` - Per-channel resolution

### System
- `vec4 iDate` - Current date (year, month, day, seconds)
- `float iSampleRate` - Audio sample rate (44100.0)

### Internal (NeoWall)
- `float _neowall_time` - Internal time uniform
- `vec2 _neowall_resolution` - Internal resolution uniform

---

## Memory Management Rules

### Always Free These:
- `neowall_shader_result_t.error_message` → `neowall_shader_free_result()`
- `shader_error_info_t` → `shader_free_error_info()`
- `shader_stats_t` → `shader_free_stats()`
- `shader_validation_t` → `shader_free_validation()`
- Any `char*` returned by utility functions → `free()`

### Never Free These:
- Template strings from `shader_get_template()`
- Template list from `shader_list_templates()`
- Version strings from `shader_get_version_string()`
- Vertex source from `shader_generate_fullscreen_vertex()`

---

## Quick Reference: Common Patterns

### Compile and Use
```c
neowall_shader_result_t r = neowall_shader_compile(src, NULL);
if (r.success) {
    glUseProgram(r.program);
    neowall_shader_set_uniforms(r.program, w, h, t);
    // render...
    neowall_shader_destroy(r.program);
} else {
    printf("Error: %s\n", r.error_message);
    neowall_shader_free_result(&r);
}
```

### Validate Before Compile
```c
shader_validation_t *v = shader_validate_syntax(src, true);
if (v->is_valid) {
    // compile...
} else {
    for (size_t i = 0; i < v->error_count; i++) {
        printf("Error: %s\n", v->errors[i]);
    }
}
shader_free_validation(v);
```

### Get Statistics
```c
shader_stats_t *s = shader_get_statistics(src);
printf("Complexity: %d%%\n", s->complexity_score);
shader_free_stats(s);
```

---

**End of API Index**