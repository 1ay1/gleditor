# Shader Library Quick Start Guide

## 🚀 Getting Started in 5 Minutes

### Basic Usage

```c
#include "shader_lib/neowall_shader_api.h"

// Compile a shader
neowall_shader_result_t result = neowall_shader_compile_file(
    "my_shader.glsl", 
    NULL  // Use default options
);

if (result.success) {
    // Use the shader
    glUseProgram(result.program);
    neowall_shader_set_uniforms(result.program, width, height, time);
    
    // ... render ...
    
    // Cleanup
    neowall_shader_destroy(result.program);
} else {
    printf("Error: %s\n", result.error_message);
    neowall_shader_free_result(&result);
}
```

## 📝 Shader Format

### Option 1: Shadertoy Format (Recommended)

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = vec4(uv, 0.5 + 0.5 * sin(iTime), 1.0);
}
```

Available uniforms:
- `iTime` - Shader playback time (seconds)
- `iResolution` - Screen resolution (vec3: width, height, aspect)
- `iMouse` - Mouse coordinates (vec4)
- `iChannel0-3` - Texture samplers
- `iFrame` - Frame counter
- `iTimeDelta` - Time between frames

### Option 2: Raw Fragment Shader

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

## 🛠️ Common Tasks

### Compile from String

```c
const char *shader = "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
                     "    fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
                     "}\n";

neowall_shader_result_t result = neowall_shader_compile(shader, NULL);
```

### Custom Options

```c
neowall_shader_options_t options = {
    .use_es3 = false,          // Use OpenGL ES 2.0
    .channel_count = 2,        // Only 2 texture channels
    .verbose_errors = true     // Detailed error messages
};

neowall_shader_result_t result = neowall_shader_compile_file(
    "shader.glsl", 
    &options
);
```

### Set Uniforms for Rendering

```c
glUseProgram(program);

// Set standard uniforms
neowall_shader_set_uniforms(program, 1920, 1080, 5.5);

// Set custom uniforms
GLint loc = glGetUniformLocation(program, "myCustomUniform");
if (loc >= 0) {
    glUniform1f(loc, 42.0f);
}
```

### Validate Before Compiling

```c
#include "shader_lib/shader_utils.h"

shader_validation_t *val = shader_validate_syntax(shader_source, true);

if (val->is_valid) {
    printf("Shader looks good!\n");
} else {
    printf("Found %zu errors:\n", val->error_count);
    for (size_t i = 0; i < val->error_count; i++) {
        printf("  - %s\n", val->errors[i]);
    }
}

shader_free_validation(val);
```

### Get Shader Statistics

```c
#include "shader_lib/shader_utils.h"

shader_stats_t *stats = shader_get_statistics(shader_source);

printf("Lines: %zu\n", stats->line_count);
printf("Uniforms: %zu\n", stats->uniform_count);
printf("Complexity: %d%%\n", stats->complexity_score);
printf("Is Shadertoy: %s\n", stats->is_shadertoy_format ? "Yes" : "No");

shader_free_stats(stats);
```

### Parse Compilation Errors

```c
#include "shader_lib/shader_utils.h"

// After compilation failure
shader_error_info_t *error = shader_parse_error_log(
    opengl_error_log, 
    shader_source
);

if (error) {
    printf("Error on line %d: %s\n", error->line_number, error->message);
    if (error->code_snippet) {
        printf("%s\n", error->code_snippet);
    }
    shader_free_error_info(error);
}
```

### Use Shader Templates

```c
#include "shader_lib/shader_utils.h"

// Get a template
const char *template = shader_get_template("plasma");

// List all templates
size_t count;
const char **templates = shader_list_templates(&count);
for (size_t i = 0; i < count; i++) {
    printf("Template: %s\n", templates[i]);
}
```

Available templates:
- `basic` - Simple gradient
- `animated` - Color cycle animation
- `plasma` - Plasma effect
- `noise` - Procedural noise
- `raymarch` - Raymarching template
- `shadertoy` - Shadertoy boilerplate

## 🎨 Rendering Loop Example

```c
GLuint program = 0;
neowall_shader_result_t result = neowall_shader_compile_file("shader.glsl", NULL);

if (!result.success) {
    fprintf(stderr, "Compilation failed: %s\n", result.error_message);
    neowall_shader_free_result(&result);
    return;
}

program = result.program;

// Set up fullscreen quad vertices
GLfloat vertices[] = {
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f
};

GLuint vbo;
glGenBuffers(1, &vbo);
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

// Render loop
float time = 0.0f;
while (running) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(program);
    neowall_shader_set_uniforms(program, width, height, time);
    
    GLint pos_attrib = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(pos_attrib);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glDisableVertexAttribArray(pos_attrib);
    
    swap_buffers();
    time += 0.016f; // 60 FPS
}

// Cleanup
glDeleteBuffers(1, &vbo);
neowall_shader_destroy(program);
```

## 🔧 Debugging

### Enable Verbose Logging

```c
// Before including shader headers:
#define SHADER_LIB_LOG_LEVEL LOG_LEVEL_DEBUG
#include "shader_lib/shader.h"
```

Or compile with:
```bash
make CFLAGS="-DSHADER_LIB_LOG_LEVEL=3"
```

### Print Shader with Line Numbers

```c
#include "shader_lib/shader_utils.h"

char *numbered = shader_add_line_numbers(shader_source, 1);
printf("%s\n", numbered);
free(numbered);
```

### Analyze Shader Complexity

```c
int score = shader_estimate_performance(shader_source);
if (score > 80) {
    printf("Warning: High complexity shader (score: %d)\n", score);
}
```

## ⚠️ Common Pitfalls

### 1. Missing Precision Qualifier

❌ **Wrong:**
```glsl
#version 100
void main() {
    gl_FragColor = vec4(1.0);
}
```

✅ **Correct:**
```glsl
#version 100
precision mediump float;
void main() {
    gl_FragColor = vec4(1.0);
}
```

### 2. Version Mismatch

❌ **Wrong:**
```glsl
#version 300 es
void main() {
    gl_FragColor = vec4(1.0);  // gl_FragColor doesn't exist in ES 3.0!
}
```

✅ **Correct:**
```glsl
#version 300 es
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0);
}
```

### 3. Forgetting to Free Memory

❌ **Wrong:**
```c
neowall_shader_result_t result = neowall_shader_compile(source, NULL);
if (!result.success) {
    return; // Memory leak!
}
```

✅ **Correct:**
```c
neowall_shader_result_t result = neowall_shader_compile(source, NULL);
if (!result.success) {
    neowall_shader_free_result(&result);
    return;
}
```

## 📚 API Reference

### High-Level API (`neowall_shader_api.h`)

| Function | Description |
|----------|-------------|
| `neowall_shader_compile()` | Compile shader from source string |
| `neowall_shader_compile_file()` | Compile shader from file |
| `neowall_shader_destroy()` | Destroy compiled program |
| `neowall_shader_free_result()` | Free result error message |
| `neowall_shader_set_uniforms()` | Set standard uniforms |
| `neowall_shader_get_vertex_source()` | Get fullscreen quad vertex shader |

### Low-Level API (`shader.h`)

| Function | Description |
|----------|-------------|
| `shader_create_program_from_sources()` | Create program from vertex + fragment |
| `shader_create_live_program()` | Create live wallpaper program from file |
| `shader_destroy_program()` | Destroy shader program |

### Utilities (`shader_utils.h`)

| Function | Description |
|----------|-------------|
| `shader_get_statistics()` | Get shader statistics |
| `shader_validate_syntax()` | Validate shader syntax |
| `shader_parse_error_log()` | Parse OpenGL error messages |
| `shader_get_template()` | Get shader template |
| `shader_format_source()` | Format shader with indentation |
| `shader_add_line_numbers()` | Add line numbers to source |
| `shader_estimate_performance()` | Estimate shader performance |

### Shadertoy Compatibility (`shadertoy_compat.h`)

| Function | Description |
|----------|-------------|
| `shadertoy_preprocess()` | Preprocess Shadertoy shader |
| `shadertoy_convert_texture_calls()` | Convert texture() to texture2D() |
| `shadertoy_analyze_shader()` | Analyze and log shader features |

## 🎯 Best Practices

1. **Always check compilation result**: Never assume compilation succeeds
2. **Free allocated memory**: Use the provided free functions
3. **Use default options first**: Only customize when needed
4. **Validate before compiling**: Catch errors early with `shader_validate_syntax()`
5. **Enable debug logging**: During development, use verbose logging
6. **Cache compiled shaders**: Don't recompile every frame
7. **Test on target hardware**: Performance varies greatly between devices

## 📖 Further Reading

- Full documentation: `src/shader_lib/README.md`
- Shadertoy examples: https://www.shadertoy.com/
- OpenGL ES reference: https://www.khronos.org/opengles/

## 💡 Examples Directory

Check these files for complete examples:
- `src/main.c` - Editor integration
- `src/shader_editor.c` - Live preview implementation

## 🐛 Troubleshooting

### Shader won't compile

1. Check for syntax errors with `shader_validate_syntax()`
2. Enable debug logging: `SHADER_LIB_LOG_LEVEL=3`
3. Print shader with line numbers
4. Try a simple template first

### Performance issues

1. Check complexity score with `shader_estimate_performance()`
2. Reduce texture lookups
3. Avoid nested loops
4. Simplify calculations

### Version errors

1. Use `shader_detect_version()` to check version
2. Let the library handle adaptation automatically
3. Or explicitly set `use_es3` in options

## 🎓 Learning Path

1. **Start simple**: Use `shader_get_template("basic")`
2. **Learn Shadertoy format**: Copy examples from shadertoy.com
3. **Add animation**: Use `iTime` uniform
4. **Experiment with patterns**: Try noise, plasma, etc.
5. **Advanced techniques**: Raymarching, procedural generation
6. **Optimize**: Use `shader_estimate_performance()` to guide optimization

## 🤝 Contributing

Found a bug? Have a suggestion? See the main project README for contribution guidelines.

---

**Happy Shader Coding! 🎨✨**