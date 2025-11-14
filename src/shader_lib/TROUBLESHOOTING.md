# Shader Compilation Troubleshooting Guide

Common shader compilation errors and how to fix them.

## Table of Contents

- [Compilation Errors](#compilation-errors)
- [Linking Errors](#linking-errors)
- [Runtime Errors](#runtime-errors)
- [Performance Issues](#performance-issues)
- [Platform-Specific Issues](#platform-specific-issues)

---

## Compilation Errors

### Error: "undeclared identifier"

**Symptom:**
```
ERROR: 0:42: 'myVariable' : undeclared identifier
```

**Causes & Solutions:**

1. **Typo in variable name**
   ```glsl
   // Wrong
   float myVaraible = 1.0;  // typo
   gl_FragColor = vec4(myVariable, 0.0, 0.0, 1.0);
   
   // Correct
   float myVariable = 1.0;
   gl_FragColor = vec4(myVariable, 0.0, 0.0, 1.0);
   ```

2. **Variable used before declaration**
   ```glsl
   // Wrong
   void main() {
       gl_FragColor = vec4(col, 1.0);
       vec3 col = vec3(1.0);  // declared after use
   }
   
   // Correct
   void main() {
       vec3 col = vec3(1.0);
       gl_FragColor = vec4(col, 1.0);
   }
   ```

3. **Missing uniform declaration**
   ```glsl
   // If using custom uniforms, declare them:
   uniform float myCustomTime;
   
   // Or use Shadertoy format which auto-declares:
   void mainImage(out vec4 fragColor, in vec2 fragCoord) {
       // iTime is automatically available
   }
   ```

---

### Error: "no matching overloaded function found"

**Symptom:**
```
ERROR: 0:15: 'texture' : no matching overloaded function found
```

**Causes & Solutions:**

1. **Using GLSL 3.0 functions in ES 2.0**
   ```glsl
   // Wrong (ES 2.0)
   #version 100
   vec4 col = texture(iChannel0, uv);
   
   // Correct (ES 2.0)
   #version 100
   vec4 col = texture2D(iChannel0, uv);
   
   // Or use ES 3.0
   #version 300 es
   vec4 col = texture(iChannel0, uv);
   ```

2. **Wrong sampler type**
   ```glsl
   // Wrong
   uniform sampler2D mySampler;
   vec4 col = texture(mySampler, vec3(uv, 0.0));  // vec3 for 2D sampler
   
   // Correct
   uniform sampler2D mySampler;
   vec4 col = texture(mySampler, uv);  // vec2 for 2D sampler
   ```

---

### Error: "syntax error"

**Symptom:**
```
ERROR: 0:20: '' : syntax error
```

**Common Causes:**

1. **Missing semicolon**
   ```glsl
   // Wrong
   float x = 1.0
   float y = 2.0;
   
   // Correct
   float x = 1.0;
   float y = 2.0;
   ```

2. **Unmatched braces**
   ```glsl
   // Wrong
   void main() {
       if (condition) {
           doSomething();
       // missing }
   }
   
   // Correct
   void main() {
       if (condition) {
           doSomething();
       }
   }
   ```

3. **Invalid preprocessor directive**
   ```glsl
   // Wrong
   #include <file.glsl>  // angle brackets not supported
   
   // Correct
   #include "file.glsl"
   ```

---

### Error: "precision qualifier missing"

**Symptom:**
```
ERROR: 0:1: '' : No precision specified for (float)
```

**Solution:**
```glsl
// Add precision qualifier at the top of fragment shader
#version 100
precision mediump float;  // or highp/lowp

void main() {
    gl_FragColor = vec4(1.0);
}
```

**Note:** Vertex shaders have default precision, fragment shaders don't.

---

### Error: "'gl_FragColor' : undeclared identifier"

**Symptom:**
```
ERROR: 0:25: 'gl_FragColor' : undeclared identifier
```

**Cause:** Using GLSL ES 3.0 which deprecated `gl_FragColor`

**Solution:**
```glsl
// GLSL ES 3.0 requires output declaration
#version 300 es
precision mediump float;

out vec4 fragColor;  // Declare output

void main() {
    fragColor = vec4(1.0);  // Use fragColor instead of gl_FragColor
}
```

---

### Error: "'varying' : syntax error"

**Symptom:**
```
ERROR: 0:10: 'varying' : syntax error
```

**Cause:** Using GLSL ES 2.0 keywords in ES 3.0

**Solution:**
```glsl
// GLSL ES 2.0
#version 100
varying vec2 vTexCoord;

// GLSL ES 3.0
#version 300 es
in vec2 vTexCoord;  // Use 'in' in fragment shaders
```

---

## Linking Errors

### Error: "Vertex shader is not compiled"

**Symptom:**
```
ERROR: Program linking failed: Vertex shader is not compiled
```

**Solution:**
Check vertex shader compilation before linking. The shader library handles this automatically, but if using low-level API:

```c
GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_src);
if (vertex == 0) {
    printf("Vertex shader compilation failed!\n");
    return false;
}

GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_src);
if (fragment == 0) {
    printf("Fragment shader compilation failed!\n");
    glDeleteShader(vertex);
    return false;
}
```

---

### Error: "Undefined reference to uniform"

**Symptom:**
```
WARNING: Uniform 'myUniform' not found in shader
```

**Causes:**

1. **Uniform not declared**
   ```glsl
   // Add uniform declaration
   uniform float myUniform;
   ```

2. **Uniform declared but not used (optimized out)**
   ```glsl
   // Uniforms that aren't used get removed by compiler
   uniform float unused;  // This will be optimized out
   
   void main() {
       // unused is never referenced
       gl_FragColor = vec4(1.0);
   }
   ```

3. **Typo in uniform name**
   ```c
   // Wrong
   glGetUniformLocation(program, "iTime");  // typo
   
   // Correct
   glGetUniformLocation(program, "_neowall_time");
   ```

---

## Runtime Errors

### Black Screen

**Possible Causes:**

1. **Shader returns zero or invalid values**
   ```glsl
   // Wrong - returns black
   void mainImage(out vec4 fragColor, in vec2 fragCoord) {
       vec3 col = vec3(0.0);  // black
       fragColor = vec4(col, 1.0);
   }
   
   // Correct - visible output
   void mainImage(out vec4 fragColor, in vec2 fragCoord) {
       vec2 uv = fragCoord / iResolution.xy;
       vec3 col = vec3(uv, 0.5);  // gradient
       fragColor = vec4(col, 1.0);
   }
   ```

2. **Alpha channel is zero**
   ```glsl
   // Wrong - transparent
   gl_FragColor = vec4(1.0, 0.0, 0.0, 0.0);
   
   // Correct - opaque
   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
   ```

3. **Division by zero**
   ```glsl
   // Add safety check
   vec2 uv = fragCoord / max(iResolution.xy, vec2(1.0));
   ```

---

### Flickering Output

**Causes:**

1. **Uninitialized variables**
   ```glsl
   // Wrong
   float value;
   if (condition) {
       value = 1.0;
   }
   // value is undefined if condition is false
   
   // Correct
   float value = 0.0;
   if (condition) {
       value = 1.0;
   }
   ```

2. **Using random/hash functions without proper seed**
   ```glsl
   // Ensure consistent hashing
   float hash(vec2 p) {
       p = fract(p * vec2(123.34, 456.21));
       p += dot(p, p + 45.32);
       return fract(p.x * p.y);
   }
   ```

---

### Wrong Colors/Unexpected Output

**Causes:**

1. **Values out of range**
   ```glsl
   // Colors should be 0.0 to 1.0
   // Wrong
   gl_FragColor = vec4(255.0, 0.0, 0.0, 1.0);  // too bright
   
   // Correct
   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
   ```

2. **Incorrect UV mapping**
   ```glsl
   // Wrong - upside down
   vec2 uv = fragCoord / iResolution.xy;
   
   // Correct - flip Y if needed
   vec2 uv = vec2(fragCoord.x, iResolution.y - fragCoord.y) / iResolution.xy;
   ```

---

## Performance Issues

### Low FPS / Lag

**Diagnosis:**
```c
int score = shader_estimate_performance(shader_source);
if (score > 80) {
    printf("High complexity shader: %d\n", score);
}
```

**Common Causes:**

1. **Too many texture lookups**
   ```glsl
   // Bad - 100 texture samples
   for (int i = 0; i < 100; i++) {
       col += texture2D(iChannel0, uv + vec2(i) * 0.01);
   }
   
   // Better - reduce samples
   for (int i = 0; i < 10; i++) {
       col += texture2D(iChannel0, uv + vec2(i) * 0.01);
   }
   ```

2. **Complex nested loops**
   ```glsl
   // Bad - O(n³)
   for (int i = 0; i < 100; i++) {
       for (int j = 0; j < 100; j++) {
           for (int k = 0; k < 100; k++) {
               // expensive
           }
       }
   }
   
   // Better - reduce iterations or break early
   for (int i = 0; i < 16; i++) {
       float d = map(pos);
       if (d < 0.001) break;
       pos += d * dir;
   }
   ```

3. **Expensive math operations**
   ```glsl
   // Bad - many expensive ops
   float result = pow(sin(x), cos(y)) * exp(sqrt(z));
   
   // Better - use simpler approximations
   float result = sin(x) * cos(y) * z;
   ```

**Solutions:**
- Reduce loop iterations
- Use lower precision (`mediump` instead of `highp`)
- Cache repeated calculations
- Use built-in functions (faster than custom)
- Reduce texture samples

---

### Shader Compilation Takes Too Long

**Causes:**
- Very large shader files
- Complex preprocessor directives
- Many includes

**Solutions:**
1. Split into multiple simpler shaders
2. Cache compiled programs
3. Minify shader code:
   ```c
   char *minified = shader_minify(shader_source);
   // Compile minified version
   ```

---

## Platform-Specific Issues

### Desktop vs Mobile

**Issue:** Shader works on desktop but fails on mobile

**Causes:**

1. **Precision issues**
   ```glsl
   // Mobile GPUs may have limited precision
   // Use mediump instead of highp
   precision mediump float;
   ```

2. **Loop limitations**
   ```glsl
   // Mobile GPUs may have loop limits
   // Use constants for loop bounds
   const int MAX_STEPS = 64;
   for (int i = 0; i < MAX_STEPS; i++) {
       // ...
   }
   ```

3. **Missing extensions**
   ```glsl
   // Check for extension support
   #ifdef GL_OES_standard_derivatives
   #extension GL_OES_standard_derivatives : enable
   #endif
   ```

---

### Linux vs Windows

**Issue:** Different OpenGL ES implementations

**Solution:**
- Test on multiple platforms
- Use portable GLSL (avoid platform-specific extensions)
- Let the shader library handle version adaptation

---

### AMD vs NVIDIA vs Intel

**Issue:** Vendor-specific quirks

**Solutions:**
1. Use standard GLSL syntax
2. Avoid vendor extensions
3. Test on multiple GPUs if possible

---

## Debug Techniques

### Enable Verbose Logging

```c
#define SHADER_LIB_LOG_LEVEL LOG_LEVEL_DEBUG
#include "shader_lib/shader.h"
```

### Print Shader with Line Numbers

```c
char *numbered = shader_add_line_numbers(shader_source, 1);
printf("%s\n", numbered);
free(numbered);
```

### Validate Before Compiling

```c
shader_validation_t *val = shader_validate_syntax(shader_source, true);
if (!val->is_valid) {
    for (size_t i = 0; i < val->error_count; i++) {
        printf("Error: %s\n", val->errors[i]);
    }
}
shader_free_validation(val);
```

### Simplify to Minimal Example

```glsl
// Start with minimal shader
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);  // Solid red
}

// If this works, gradually add complexity
```

### Use Color for Debugging

```glsl
// Visualize values with color
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    
    // Debug: show UV coordinates as color
    fragColor = vec4(uv, 0.0, 1.0);
    
    // Debug: show time as brightness
    // fragColor = vec4(vec3(fract(iTime)), 1.0);
    
    // Debug: show conditionals
    // fragColor = condition ? vec4(1,0,0,1) : vec4(0,1,0,1);
}
```

---

## Quick Fixes Checklist

When shader won't compile:

- [ ] Check for missing semicolons
- [ ] Verify all braces match `{}` 
- [ ] Add precision qualifier (`precision mediump float;`)
- [ ] Check GLSL version matches syntax
- [ ] Verify all variables are declared
- [ ] Check for typos in variable names
- [ ] Ensure `main()` or `mainImage()` exists
- [ ] Validate uniform declarations
- [ ] Check for unsupported GLSL features
- [ ] Try a simple test shader first

When shader compiles but doesn't render:

- [ ] Check alpha channel (should be 1.0)
- [ ] Verify output color is in range 0-1
- [ ] Check for division by zero
- [ ] Verify uniforms are set correctly
- [ ] Test with solid color first
- [ ] Check viewport/resolution settings
- [ ] Verify OpenGL state (depth test, blending, etc.)

---

## Getting Help

If you're still stuck:

1. **Check the logs:** Enable debug logging
2. **Simplify:** Reduce to minimal failing example
3. **Validate:** Use `shader_validate_syntax()`
4. **Compare:** Test with a working template
5. **Search:** Look for similar errors online
6. **Ask:** Include full error message and shader code

---

## Additional Resources

- Shader Library API: `API_INDEX.md`
- Quick Start Guide: `QUICKSTART.md`
- Full Documentation: `README.md`
- GLSL Reference: https://www.khronos.org/opengl/wiki/OpenGL_Shading_Language
- Shadertoy: https://www.shadertoy.com/

---

**Remember:** Most shader errors are typos, missing declarations, or version mismatches. Start simple and build up complexity gradually!