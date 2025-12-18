/* Shader Multipass Support - Implementation
 * Implements Shadertoy-style multipass rendering with BufferA-D and Image passes
 * 
 * This is a self-contained shader compilation and rendering system.
 * No legacy dependencies required.
 */

#include "shader_multipass.h"
#include "shader_log.h"
#include "platform_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

/* ============================================
 * Error Logging for Shader Compilation
 * ============================================ */

#define MAX_ERROR_LOG_SIZE 16384
static char g_last_error_log[MAX_ERROR_LOG_SIZE];
static size_t g_error_log_pos = 0;

static void clear_error_log(void) {
    g_last_error_log[0] = '\0';
    g_error_log_pos = 0;
}

static void append_to_error_log(const char *format, ...) {
    if (g_error_log_pos >= MAX_ERROR_LOG_SIZE - 1) return;
    
    va_list args;
    va_start(args, format);
    int written = vsnprintf(g_last_error_log + g_error_log_pos,
                           MAX_ERROR_LOG_SIZE - g_error_log_pos,
                           format, args);
    va_end(args);
    
    if (written > 0) {
        g_error_log_pos += written;
        if (g_error_log_pos >= MAX_ERROR_LOG_SIZE) {
            g_error_log_pos = MAX_ERROR_LOG_SIZE - 1;
        }
    }
}

const char *multipass_get_error_log(void) {
    return g_last_error_log;
}

/* ============================================
 * Shader Compilation Utilities
 * ============================================ */

static void print_shader_with_line_numbers(const char *source, const char *type) {
    if (!source) return;
    
    log_debug("========== %s SHADER SOURCE (with line numbers) ==========", type);
    
    const char *line_start = source;
    const char *line_end;
    int line_num = 1;
    
    while (*line_start) {
        line_end = strchr(line_start, '\n');
        if (line_end) {
            log_debug("%4d: %.*s", line_num, (int)(line_end - line_start), line_start);
            line_start = line_end + 1;
        } else {
            log_debug("%4d: %s", line_num, line_start);
            break;
        }
        line_num++;
    }
    
    log_debug("========== END %s SHADER SOURCE ==========", type);
}

static GLuint compile_shader(GLenum type, const char *source) {
    const char *type_str = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
    
    print_shader_with_line_numbers(source, type_str);
    
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        log_error("Failed to create %s shader", type_str);
        append_to_error_log("ERROR: Failed to create %s shader\n", type_str);
        return 0;
    }
    
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        append_to_error_log("\n=== %s SHADER COMPILATION FAILED ===\n\n",
                           (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT");
        
        GLint info_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1) {
            char *info_log = malloc(info_len);
            if (info_log) {
                glGetShaderInfoLog(shader, info_len, NULL, info_log);
                log_error("%s shader compilation failed: %s", type_str, info_log);
                append_to_error_log("%s\n", info_log);
                free(info_log);
            }
        }
        
        glDeleteShader(shader);
        return 0;
    }
    
    log_debug("%s shader compiled successfully", type_str);
    return shader;
}

static bool shader_create_program_from_sources(const char *vertex_src,
                                                const char *fragment_src,
                                                GLuint *program) {
    if (!program) {
        log_error("Invalid program pointer");
        return false;
    }
    
    clear_error_log();
    
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_src);
    if (vertex_shader == 0) {
        return false;
    }
    
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_src);
    if (fragment_shader == 0) {
        glDeleteShader(vertex_shader);
        return false;
    }
    
    GLuint prog = glCreateProgram();
    if (prog == 0) {
        log_error("Failed to create shader program");
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    
    glAttachShader(prog, vertex_shader);
    glAttachShader(prog, fragment_shader);
    glLinkProgram(prog);
    
    GLint linked;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        append_to_error_log("\n=== PROGRAM LINKING FAILED ===\n\n");
        
        GLint info_len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1) {
            char *info_log = malloc(info_len);
            if (info_log) {
                glGetProgramInfoLog(prog, info_len, NULL, info_log);
                log_error("Program linking failed: %s", info_log);
                append_to_error_log("%s\n", info_log);
                free(info_log);
            }
        }
        
        glDeleteProgram(prog);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    *program = prog;
    log_debug("Shader program created successfully (ID: %u)", prog);
    return true;
}



/* ============================================
 * Internal Helper Functions
 * ============================================ */

/* Skip whitespace (currently unused but kept for future use) */
/*
static const char *skip_whitespace(const char *p) {
    while (*p && isspace(*p)) p++;
    return p;
}
*/

/* Check if character is valid identifier char (currently unused but kept for future use) */
/*
static bool is_ident_char(char c) {
    return isalnum(c) || c == '_';
}
*/

/* Find next occurrence of pattern, respecting comments */
static const char *find_pattern(const char *source, const char *pattern) {
    const char *p = source;
    size_t pat_len = strlen(pattern);

    while (*p) {
        /* Skip single-line comments */
        if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }

        /* Skip multi-line comments */
        if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) p++;
            if (*p) p += 2;
            continue;
        }

        /* Check for pattern */
        if (strncmp(p, pattern, pat_len) == 0) {
            return p;
        }

        p++;
    }

    return NULL;
}

/* Find the end of a function body (matching closing brace) */
static const char *find_function_end(const char *start) {
    const char *p = start;
    int brace_depth = 0;
    bool in_function = false;

    while (*p) {
        /* Skip comments */
        if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) p++;
            if (*p) p += 2;
            continue;
        }

        /* Skip strings */
        if (*p == '"') {
            p++;
            while (*p && *p != '"') {
                if (*p == '\\' && p[1]) p++;
                p++;
            }
            if (*p) p++;
            continue;
        }

        if (*p == '{') {
            brace_depth++;
            in_function = true;
        } else if (*p == '}') {
            brace_depth--;
            if (in_function && brace_depth == 0) {
                return p + 1;
            }
        }

        p++;
    }

    return p;
}

/* Extract a substring */
static char *extract_substring(const char *start, const char *end) {
    if (!start || !end || end <= start) return NULL;

    size_t len = end - start;
    char *result = malloc(len + 1);
    if (!result) return NULL;

    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

/* Duplicate a string */
static char *str_dup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *result = malloc(len + 1);
    if (result) {
        memcpy(result, s, len + 1);
    }
    return result;
}

/* ============================================
 * Pass Type Utilities
 * ============================================ */

const char *multipass_type_name(multipass_type_t type) {
    switch (type) {
        case PASS_TYPE_BUFFER_A: return "Buffer A";
        case PASS_TYPE_BUFFER_B: return "Buffer B";
        case PASS_TYPE_BUFFER_C: return "Buffer C";
        case PASS_TYPE_BUFFER_D: return "Buffer D";
        case PASS_TYPE_IMAGE:    return "Image";
        case PASS_TYPE_COMMON:   return "Common";
        case PASS_TYPE_SOUND:    return "Sound";
        default:                 return "None";
    }
}

multipass_type_t multipass_type_from_name(const char *name) {
    if (!name) return PASS_TYPE_NONE;

    /* Case-insensitive comparison */
    if (strcasecmp(name, "Buffer A") == 0 || strcasecmp(name, "BufferA") == 0)
        return PASS_TYPE_BUFFER_A;
    if (strcasecmp(name, "Buffer B") == 0 || strcasecmp(name, "BufferB") == 0)
        return PASS_TYPE_BUFFER_B;
    if (strcasecmp(name, "Buffer C") == 0 || strcasecmp(name, "BufferC") == 0)
        return PASS_TYPE_BUFFER_C;
    if (strcasecmp(name, "Buffer D") == 0 || strcasecmp(name, "BufferD") == 0)
        return PASS_TYPE_BUFFER_D;
    if (strcasecmp(name, "Image") == 0)
        return PASS_TYPE_IMAGE;
    if (strcasecmp(name, "Common") == 0)
        return PASS_TYPE_COMMON;
    if (strcasecmp(name, "Sound") == 0)
        return PASS_TYPE_SOUND;

    return PASS_TYPE_NONE;
}

const char *multipass_channel_source_name(channel_source_t source) {
    switch (source) {
        case CHANNEL_SOURCE_BUFFER_A: return "Buffer A";
        case CHANNEL_SOURCE_BUFFER_B: return "Buffer B";
        case CHANNEL_SOURCE_BUFFER_C: return "Buffer C";
        case CHANNEL_SOURCE_BUFFER_D: return "Buffer D";
        case CHANNEL_SOURCE_TEXTURE:  return "Texture";
        case CHANNEL_SOURCE_KEYBOARD: return "Keyboard";
        case CHANNEL_SOURCE_NOISE:    return "Noise";
        case CHANNEL_SOURCE_SELF:     return "Self";
        default:                      return "None";
    }
}

multipass_channel_t multipass_default_channel(channel_source_t source) {
    multipass_channel_t channel = {
        .source = source,
        .texture_id = 0,
        .vflip = false,
        .filter = GL_LINEAR,
        .wrap = GL_CLAMP_TO_EDGE
    };
    return channel;
}

/* ============================================
 * Shader Parsing Functions
 * ============================================ */

int multipass_count_main_functions(const char *source) {
    if (!source) return 0;

    int count = 0;
    const char *p = source;

    while ((p = find_pattern(p, "mainImage")) != NULL) {
        /* Check if it's actually a function definition */
        const char *before = p;
        if (before > source) {
            before--;
            while (before > source && isspace(*before)) before--;
        }

        /* Skip past "mainImage" */
        p += 9;

        /* Skip whitespace */
        while (*p && isspace(*p)) p++;

        /* Must be followed by '(' */
        if (*p == '(') {
            count++;
        }
    }

    return count;
}

bool multipass_detect(const char *source) {
    if (!source) return false;

    /*
     * All shaders go through the multipass system now.
     * Single-pass shaders are treated as Image-only multipass.
     * This simplifies the codebase by removing the legacy single-pass path.
     */
    int main_count = multipass_count_main_functions(source);
    if (main_count >= 1) {
        return true;
    }

    /* Check for mainImage function */
    if (find_pattern(source, "void mainImage") ||
        find_pattern(source, "void main(")) {
        return true;
    }

    return false;
}

char *multipass_extract_common(const char *source) {
    if (!source) return NULL;

    /* Find start of first mainImage function */
    const char *first_main = find_pattern(source, "void mainImage");
    if (!first_main) {
        first_main = find_pattern(source, "void main(");
    }

    if (!first_main) {
        return NULL;
    }

    /* Go back to find the start of function (might have return type, etc.) */
    const char *func_start = first_main;
    while (func_start > source && *(func_start - 1) != '\n') {
        func_start--;
    }

    /* Everything before the first function is common code */
    if (func_start > source) {
        return extract_substring(source, func_start);
    }

    return NULL;
}

multipass_parse_result_t *multipass_parse_shader(const char *source) {
    multipass_parse_result_t *result = calloc(1, sizeof(multipass_parse_result_t));
    if (!result) return NULL;

    if (!source) {
        result->error_message = str_dup("Source is NULL");
        return result;
    }

    int main_count = multipass_count_main_functions(source);

    if (main_count <= 1) {
        /* Single pass shader */
        result->is_multipass = false;
        result->pass_count = 1;
        result->pass_sources[0] = str_dup(source);
        result->pass_types[0] = PASS_TYPE_IMAGE;
        return result;
    }

    result->is_multipass = true;
    log_info("Detected multipass shader with %d mainImage functions", main_count);

    /* Extract common code (everything before first mainImage) */
    result->common_source = multipass_extract_common(source);

    /*
     * MULTIPASS EXTRACTION STRATEGY:
     *
     * For shaders with multiple mainImage functions, we need to:
     * 1. Extract each mainImage function separately
     * 2. Include helper functions that appear BETWEEN mainImage functions
     *    with the passes that need them (but NOT other mainImage functions)
     *
     * Example: If shader has mainImage1, helperFunc, mainImage2, helperFunc2, mainImage3
     * - Pass 0: mainImage1 only
     * - Pass 1: helperFunc + mainImage2
     * - Pass 2: helperFunc + helperFunc2 + mainImage3
     */

    /* First, find all mainImage positions and their function boundaries */
    const char *main_starts[MULTIPASS_MAX_PASSES];  /* Start of "void mainImage" */
    const char *main_ends[MULTIPASS_MAX_PASSES];    /* End of mainImage function body */
    const char *line_starts[MULTIPASS_MAX_PASSES];  /* Start of line containing mainImage */
    int found_count = 0;
    
    (void)main_starts; /* Currently unused but kept for future use */

    const char *p = source;
    while (found_count < MULTIPASS_MAX_PASSES) {
        const char *main_start = find_pattern(p, "void mainImage");
        if (!main_start) break;

        /* Find start of the line */
        const char *line_start = main_start;
        while (line_start > source && *(line_start - 1) != '\n') {
            line_start--;
        }

        main_starts[found_count] = main_start;
        line_starts[found_count] = line_start;
        main_ends[found_count] = find_function_end(main_start);
        found_count++;
        p = main_ends[found_count - 1];
    }

    /* Now extract each pass with proper helper function inclusion */
    for (int pass_index = 0; pass_index < found_count; pass_index++) {
        const char *line_start = line_starts[pass_index];
        const char *func_end = main_ends[pass_index];

        /* Check for pass marker in preceding lines */
        multipass_type_t detected_type = PASS_TYPE_NONE;
        const char *check = line_start;
        int lines_back = 0;
        while (check > source && lines_back < 5) {
            /* Go to previous line */
            check--;
            while (check > source && *(check - 1) != '\n') check--;

            /* Check this line for markers - be more specific to avoid false positives */
            /* Only check comment lines */
            const char *line_content = check;
            while (*line_content && isspace(*line_content)) line_content++;

            if (line_content[0] == '/' && (line_content[1] == '/' || line_content[1] == '*')) {
                if (strstr(check, "Buffer A") || strstr(check, "BufferA")) {
                    detected_type = PASS_TYPE_BUFFER_A;
                    break;
                } else if (strstr(check, "Buffer B") || strstr(check, "BufferB")) {
                    detected_type = PASS_TYPE_BUFFER_B;
                    break;
                } else if (strstr(check, "Buffer C") || strstr(check, "BufferC")) {
                    detected_type = PASS_TYPE_BUFFER_C;
                    break;
                } else if (strstr(check, "Buffer D") || strstr(check, "BufferD")) {
                    detected_type = PASS_TYPE_BUFFER_D;
                    break;
                } else if (strstr(check, "// Image") || strstr(check, "/* Image")) {
                    detected_type = PASS_TYPE_IMAGE;
                    break;
                }
            }

            lines_back++;
        }

        /*
         * Default assignment based on order if no marker found:
         * - For 2 passes: Buffer A, Image
         * - For 3 passes: Buffer A, Buffer B, Image
         * - For 4 passes: Buffer A, Buffer B, Buffer C, Image
         * - etc.
         * The LAST pass is always Image, all others are Buffers A, B, C, D
         */
        if (detected_type == PASS_TYPE_NONE) {
            if (pass_index == found_count - 1) {
                detected_type = PASS_TYPE_IMAGE;  /* Last pass is always Image */
            } else {
                /* Assign buffers A, B, C, D in order */
                detected_type = PASS_TYPE_BUFFER_A + pass_index;
                if (detected_type > PASS_TYPE_BUFFER_D) {
                    detected_type = PASS_TYPE_BUFFER_D;  /* Cap at Buffer D */
                }
            }
        }

        log_info("Pass %d assigned type: %s", pass_index, multipass_type_name(detected_type));

        /*
         * For passes after the first one, include ALL helper functions defined
         * between the FIRST mainImage end and THIS mainImage start.
         * This ensures functions like makeBloom() (defined between pass 0 and 1)
         * are available to pass 2 as well.
         */
        if (pass_index > 0) {
            /* Get ALL helper code from end of FIRST mainImage to start of THIS mainImage */
            const char *helpers_start = main_ends[0];  /* After first mainImage */
            const char *helpers_end = line_start;

            /* We need to EXCLUDE other mainImage functions from the helpers */
            /* Build a string with only the helper functions */
            size_t max_helpers_len = (helpers_end > helpers_start) ? (helpers_end - helpers_start) : 0;
            char *helpers_only = NULL;
            size_t helpers_only_len = 0;

            if (max_helpers_len > 0) {
                helpers_only = malloc(max_helpers_len + 1);
                if (helpers_only) {
                    helpers_only[0] = '\0';
                    helpers_only_len = 0;

                    /* Copy code between each mainImage, skipping the mainImage functions themselves */
                    for (int prev = 0; prev < pass_index; prev++) {
                        const char *seg_start = main_ends[prev];
                        const char *seg_end = line_starts[prev + 1];

                        if (seg_end > seg_start) {
                            size_t seg_len = seg_end - seg_start;
                            memcpy(helpers_only + helpers_only_len, seg_start, seg_len);
                            helpers_only_len += seg_len;
                        }
                    }
                    helpers_only[helpers_only_len] = '\0';
                }
            }

            /* Calculate sizes for final combined source */
            size_t main_len = func_end - line_start;
            size_t total_len = helpers_only_len + main_len + 16;

            char *combined = malloc(total_len);
            if (combined) {
                combined[0] = '\0';

                /* Add accumulated helper functions */
                if (helpers_only && helpers_only_len > 0) {
                    strcat(combined, helpers_only);
                }

                /* Add this mainImage function */
                strncat(combined, line_start, main_len);

                result->pass_sources[pass_index] = combined;
            } else {
                result->pass_sources[pass_index] = extract_substring(line_start, func_end);
            }

            free(helpers_only);
        } else {
            /* First pass - just extract the mainImage function */
            result->pass_sources[pass_index] = extract_substring(line_start, func_end);
        }

        result->pass_types[pass_index] = detected_type;

        log_info("Extracted pass %d: %s", pass_index, multipass_type_name(detected_type));
    }

    result->pass_count = found_count;

    return result;
}

void multipass_free_parse_result(multipass_parse_result_t *result) {
    if (!result) return;

    for (int i = 0; i < MULTIPASS_MAX_PASSES; i++) {
        free(result->pass_sources[i]);
    }
    free(result->common_source);
    free(result->error_message);
    free(result);
}

/* ============================================
 * Shader wrapper for each pass
 * ============================================ */

/* Shadertoy wrapper prefix for ES 3.0 / Desktop OpenGL */
static const char *multipass_wrapper_prefix =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "\n"
    "// Shadertoy compatibility uniforms\n"
    "uniform float iTime;\n"
    "uniform vec3 iResolution;\n"
    "uniform vec4 iMouse;\n"
    "uniform int iFrame;\n"
    "uniform float iTimeDelta;\n"
    "uniform float iFrameRate;\n"
    "uniform vec4 iDate;\n"
    "uniform float iSampleRate;\n"
    "\n"
    "// Texture samplers\n"
    "uniform sampler2D iChannel0;\n"
    "uniform sampler2D iChannel1;\n"
    "uniform sampler2D iChannel2;\n"
    "uniform sampler2D iChannel3;\n"
    "\n"
    "// Channel resolutions\n"
    "uniform vec3 iChannelResolution[4];\n"
    "uniform float iChannelTime[4];\n"
    "\n"
    "// Output\n"
    "out vec4 fragColor;\n"
    "\n"
    "// Note: tanh is built-in for GLSL ES 3.0+, no polyfill needed\n"
    "\n";

static const char *multipass_wrapper_suffix =
    "\n"
    "void main() {\n"
    "    mainImage(fragColor, gl_FragCoord.xy);\n"
    "}\n";

/**
 * Fix common Shadertoy compatibility issues in shader source.
 * 
 * Handles:
 * - iChannelResolution[n] used as vec2 (add .xy swizzle)
 * - texture(sampler, vec3) -> texture(sampler, (vec3).xy) for 2D textures
 * - Other implicit vec3->vec2 casts
 * 
 * Returns a newly allocated string that must be freed.
 */
static char *fix_shadertoy_compatibility(const char *source) {
    if (!source) return NULL;
    
    size_t src_len = strlen(source);
    /* Allocate extra space for potential .xy additions */
    size_t alloc_size = src_len * 3 + 1;
    char *result = malloc(alloc_size);
    if (!result) return NULL;
    
    const char *src = source;
    char *dst = result;
    
    while (*src) {
        /* Check for iChannelResolution[n] pattern */
        if (strncmp(src, "iChannelResolution[", 19) == 0) {
            /* Copy "iChannelResolution[" */
            memcpy(dst, src, 19);
            dst += 19;
            src += 19;
            
            /* Copy the index (digit) */
            while (*src && *src != ']') {
                *dst++ = *src++;
            }
            
            /* Copy the closing bracket */
            if (*src == ']') {
                *dst++ = *src++;
            }
            
            /* Check if already followed by a swizzle or component access */
            if (*src != '.' && *src != '[') {
                /* Add .xy swizzle for vec3->vec2 compatibility */
                memcpy(dst, ".xy", 3);
                dst += 3;
            }
            continue;
        }
        
        /* Check for texture(iChannel, expr) where expr might be vec3 */
        /* Pattern: "texture(iChannel" followed by digit, comma, then expression */
        if (strncmp(src, "texture(iChannel", 16) == 0) {
            /* Copy "texture(iChannel" */
            memcpy(dst, src, 16);
            dst += 16;
            src += 16;
            
            /* Copy the channel number */
            while (*src && *src >= '0' && *src <= '9') {
                *dst++ = *src++;
            }
            
            /* Skip whitespace and comma */
            while (*src && (*src == ' ' || *src == '\t')) {
                *dst++ = *src++;
            }
            if (*src == ',') {
                *dst++ = *src++;
            }
            while (*src && (*src == ' ' || *src == '\t')) {
                *dst++ = *src++;
            }
            
            /* Now we're at the coordinate expression */
            /* Check if it starts with something that's likely a vec3 */
            /* Common patterns: variable name, function call, or expression */
            
            /* We need to find the end of this expression (the closing paren or next comma) */
            /* and wrap it with parentheses + .xy if it doesn't already have .xy */
            
            /* Count parentheses to find the expression end */
            int paren_depth = 1; /* We're inside texture( */
            const char *expr_start = src;
            const char *expr_end = src;
            bool has_swizzle = false;
            
            while (*expr_end && paren_depth > 0) {
                if (*expr_end == '(') paren_depth++;
                else if (*expr_end == ')') paren_depth--;
                else if (*expr_end == ',' && paren_depth == 1) break; /* Next argument */
                
                /* Check for .xy, .xz, .yz etc swizzle at end of expression */
                if (*expr_end == '.' && paren_depth == 1) {
                    const char *after_dot = expr_end + 1;
                    if ((*after_dot == 'x' || *after_dot == 'y' || *after_dot == 'z' || 
                         *after_dot == 'r' || *after_dot == 'g' || *after_dot == 'b' ||
                         *after_dot == 's' || *after_dot == 't' || *after_dot == 'p')) {
                        has_swizzle = true;
                    }
                }
                expr_end++;
            }
            
            /* Back up to the actual end of the expression */
            if (paren_depth == 0) expr_end--;
            while (expr_end > expr_start && (*(expr_end-1) == ' ' || *(expr_end-1) == '\t')) {
                expr_end--;
            }
            
            /* Copy the expression */
            size_t expr_len = expr_end - expr_start;
            if (!has_swizzle && expr_len > 0) {
                /* Wrap with parentheses and add .xy */
                *dst++ = '(';
                memcpy(dst, expr_start, expr_len);
                dst += expr_len;
                memcpy(dst, ").xy", 4);
                dst += 4;
            } else {
                /* Already has swizzle, copy as-is */
                memcpy(dst, expr_start, expr_len);
                dst += expr_len;
            }
            
            src = expr_end;
            continue;
        }
        
        /* Copy character as-is */
        *dst++ = *src++;
    }
    
    *dst = '\0';
    return result;
}

/* Wrap a pass source with Shadertoy compatibility layer */
static char *wrap_pass_source(const char *common, const char *pass_source) {
    size_t prefix_len = strlen(multipass_wrapper_prefix);
    size_t common_len = common ? strlen(common) : 0;
    size_t pass_len = pass_source ? strlen(pass_source) : 0;
    size_t suffix_len = strlen(multipass_wrapper_suffix);

    /* Extra space for .xy additions (worst case: every iChannelResolution gets .xy) */
    size_t total = prefix_len + (common_len * 2) + (pass_len * 2) + suffix_len + 64;
    char *wrapped = malloc(total);
    if (!wrapped) return NULL;

    wrapped[0] = '\0';
    strcat(wrapped, multipass_wrapper_prefix);
    
    /* Apply compatibility fixes to common code */
    if (common) {
        char *fixed_common = fix_shadertoy_compatibility(common);
        if (fixed_common) {
            strcat(wrapped, fixed_common);
            free(fixed_common);
        } else {
            strcat(wrapped, common);
        }
    }
    strcat(wrapped, "\n");
    
    /* Apply compatibility fixes to pass source */
    if (pass_source) {
        char *fixed_pass = fix_shadertoy_compatibility(pass_source);
        if (fixed_pass) {
            strcat(wrapped, fixed_pass);
            free(fixed_pass);
        } else {
            strcat(wrapped, pass_source);
        }
    }
    strcat(wrapped, multipass_wrapper_suffix);

    return wrapped;
}

/* Vertex shader for fullscreen quad */
static const char *fullscreen_vertex_shader =
    "#version 300 es\n"
    "in vec2 position;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

/* ============================================
 * Multipass Shader Creation
 * ============================================ */

multipass_shader_t *multipass_create(const char *source) {
    multipass_parse_result_t *parsed = multipass_parse_shader(source);
    if (!parsed) return NULL;

    multipass_shader_t *shader = multipass_create_from_parsed(parsed);
    multipass_free_parse_result(parsed);

    return shader;
}

multipass_shader_t *multipass_create_from_parsed(const multipass_parse_result_t *parse_result) {
    if (!parse_result) return NULL;

    multipass_shader_t *shader = calloc(1, sizeof(multipass_shader_t));
    if (!shader) return NULL;

    shader->common_source = parse_result->common_source ?
                            str_dup(parse_result->common_source) : NULL;
    shader->pass_count = parse_result->pass_count;
    shader->image_pass_index = -1;
    shader->has_buffers = false;

    for (int i = 0; i < parse_result->pass_count; i++) {
        multipass_pass_t *pass = &shader->passes[i];

        pass->type = parse_result->pass_types[i];
        pass->name = str_dup(multipass_type_name(pass->type));
        pass->source = str_dup(parse_result->pass_sources[i]);
        pass->is_compiled = false;

        /*
         * GENERAL SHADERTOY CHANNEL BINDING
         *
         * Shadertoy channel bindings are defined in JSON metadata which we don't have.
         * We use the most common Shadertoy pattern:
         *
         *   iChannel0 = Self (previous frame feedback - most common for temporal effects)
         *   iChannel1 = Buffer A (or previous buffer in chain)
         *   iChannel2 = Buffer B
         *   iChannel3 = Buffer C
         *
         * This works for the majority of Shadertoy shaders because:
         * - Feedback/temporal effects almost always use iChannel0
         * - Scene/input data typically comes from iChannel1
         * - Additional buffers are on higher channels
         *
         * Passes are rendered in order: Buffer A -> B -> C -> D -> Image
         */

        if (pass->type == PASS_TYPE_IMAGE) {
            shader->image_pass_index = i;
            /* Image pass reads from all buffers, no self */
            pass->channels[0].source = CHANNEL_SOURCE_BUFFER_A;
            pass->channels[1].source = CHANNEL_SOURCE_BUFFER_B;
            pass->channels[2].source = CHANNEL_SOURCE_BUFFER_C;
            pass->channels[3].source = CHANNEL_SOURCE_BUFFER_D;
        } else {
            shader->has_buffers = true;
            /* Buffer passes: iChannel0=self, iChannel1-3=previous buffers */
            pass->channels[0].source = CHANNEL_SOURCE_SELF;
            pass->channels[1].source = CHANNEL_SOURCE_BUFFER_A;
            pass->channels[2].source = CHANNEL_SOURCE_BUFFER_B;
            pass->channels[3].source = CHANNEL_SOURCE_BUFFER_C;
        }

        const char* src_names[] = {"None", "BufA", "BufB", "BufC", "BufD", "Tex", "Kbd", "Noise", "Self"};
        log_info("  Pass %d (%s): ch0=%s, ch1=%s, ch2=%s, ch3=%s",
                 i, pass->name,
                 src_names[pass->channels[0].source],
                 src_names[pass->channels[1].source],
                 src_names[pass->channels[2].source],
                 src_names[pass->channels[3].source]);
    }

    log_info("Created multipass shader with %d passes (has_buffers=%d, image_index=%d)",
             shader->pass_count, shader->has_buffers, shader->image_pass_index);

    return shader;
}

bool multipass_init_gl(multipass_shader_t *shader, int width, int height) {
    if (!shader) return false;

    if (shader->is_initialized) {
        log_debug("Multipass GL already initialized");
        return true;
    }

    log_info("Initializing multipass GL resources (%dx%d)", width, height);

    /* Get the default framebuffer ID (GTK may use non-zero FBO) */
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &shader->default_framebuffer);
    log_info("Default framebuffer ID: %d", shader->default_framebuffer);

    /* Create VAO and VBO for fullscreen quad */
    static const float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };

#if defined(HAVE_GLES3) || defined(USE_EPOXY)
    glGenVertexArrays(1, &shader->vao);
    glBindVertexArray(shader->vao);
#endif

    glGenBuffers(1, &shader->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, shader->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* Generate noise texture */
    glGenTextures(1, &shader->noise_texture);
    glBindTexture(GL_TEXTURE_2D, shader->noise_texture);

    unsigned char *noise_data = malloc(256 * 256 * 4);
    if (noise_data) {
        for (int i = 0; i < 256 * 256 * 4; i++) {
            noise_data[i] = rand() % 256;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, noise_data);
        free(noise_data);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    /* Initialize each pass */
    for (int i = 0; i < shader->pass_count; i++) {
        multipass_pass_t *pass = &shader->passes[i];
        pass->width = width;
        pass->height = height;
        pass->ping_pong_index = 0;
        pass->needs_clear = true;

        /* Create FBO and textures for buffer passes */
        if (pass->type >= PASS_TYPE_BUFFER_A && pass->type <= PASS_TYPE_BUFFER_D) {
            glGenFramebuffers(1, &pass->fbo);
            glGenTextures(2, pass->textures);

            for (int t = 0; t < 2; t++) {
                glBindTexture(GL_TEXTURE_2D, pass->textures[t]);
                /* Use GL_RGBA16F for HDR support:
                 * - Bloom requires values > 1.0 to pass BLOOM_THRESHOLD
                 * - This matches Shadertoy's actual buffer behavior */
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                            GL_RGBA, GL_FLOAT, NULL);
                /* Use LINEAR_MIPMAP_LINEAR for textureLod support (bloom needs this) */
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                /* Pre-allocate mipmap levels */
                glGenerateMipmap(GL_TEXTURE_2D);
            }

            log_info("Created FBO and textures for %s", pass->name);
        }
    }

    shader->is_initialized = true;
    shader->frame_count = 0;

    return true;
}

bool multipass_compile_pass(multipass_shader_t *shader, int pass_index) {
    if (!shader || pass_index < 0 || pass_index >= shader->pass_count) {
        return false;
    }

    multipass_pass_t *pass = &shader->passes[pass_index];

    log_info("Compiling pass %d: %s", pass_index, pass->name);

    /* Clean up previous compilation */
    if (pass->program) {
        glDeleteProgram(pass->program);
        pass->program = 0;
    }
    if (pass->compile_error) {
        free(pass->compile_error);
        pass->compile_error = NULL;
    }

    /* Wrap pass source with compatibility layer */
    char *wrapped = wrap_pass_source(shader->common_source, pass->source);
    if (!wrapped) {
        pass->compile_error = str_dup("Failed to allocate memory for shader wrapping");
        pass->is_compiled = false;
        return false;
    }

    /* Compile shaders */
    GLuint program;
    bool success = shader_create_program_from_sources(fullscreen_vertex_shader, wrapped, &program);

    free(wrapped);

    if (!success) {
        const char *error_log = multipass_get_error_log();
        pass->compile_error = str_dup(error_log ? error_log : "Unknown compilation error");
        pass->is_compiled = false;
        log_error("Failed to compile pass %s: %s", pass->name, pass->compile_error);
        return false;
    }

    pass->program = program;
    pass->is_compiled = true;

    log_info("Successfully compiled pass %s (program=%u)", pass->name, program);

    return true;
}

bool multipass_compile_all(multipass_shader_t *shader) {
    if (!shader) return false;

    bool all_success = true;

    for (int i = 0; i < shader->pass_count; i++) {
        if (!multipass_compile_pass(shader, i)) {
            all_success = false;
        }
    }

    return all_success;
}

void multipass_resize(multipass_shader_t *shader, int width, int height) {
    if (!shader || !shader->is_initialized) return;

    for (int i = 0; i < shader->pass_count; i++) {
        multipass_pass_t *pass = &shader->passes[i];

        if (pass->width == width && pass->height == height) {
            continue;
        }

        pass->width = width;
        pass->height = height;

        /* Resize buffer textures */
        if (pass->type >= PASS_TYPE_BUFFER_A && pass->type <= PASS_TYPE_BUFFER_D) {
            for (int t = 0; t < 2; t++) {
                glBindTexture(GL_TEXTURE_2D, pass->textures[t]);
                /* Use GL_RGBA16F for HDR bloom support */
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                            GL_RGBA, GL_FLOAT, NULL);
                /* Regenerate mipmaps for new size */
                glGenerateMipmap(GL_TEXTURE_2D);
            }
            pass->needs_clear = true;
        }
    }
}

void multipass_destroy(multipass_shader_t *shader) {
    if (!shader) return;

    /* Delete passes */
    for (int i = 0; i < shader->pass_count; i++) {
        multipass_pass_t *pass = &shader->passes[i];

        if (pass->program) glDeleteProgram(pass->program);
        if (pass->fbo) glDeleteFramebuffers(1, &pass->fbo);
        if (pass->textures[0]) glDeleteTextures(2, pass->textures);

        free(pass->name);
        free(pass->source);
        free(pass->compile_error);
    }

    /* Delete shared resources */
    if (shader->vbo) glDeleteBuffers(1, &shader->vbo);
#if defined(HAVE_GLES3) || defined(USE_EPOXY)
    if (shader->vao) glDeleteVertexArrays(1, &shader->vao);
#endif
    if (shader->noise_texture) glDeleteTextures(1, &shader->noise_texture);
    if (shader->keyboard_texture) glDeleteTextures(1, &shader->keyboard_texture);

    free(shader->common_source);
    free(shader);
}

/* ============================================
 * Rendering Functions
 * ============================================ */

void multipass_set_uniforms(multipass_shader_t *shader,
                            int pass_index,
                            float shader_time,
                            float mouse_x, float mouse_y,
                            bool mouse_click) {
    if (!shader || pass_index < 0 || pass_index >= shader->pass_count) return;

    multipass_pass_t *pass = &shader->passes[pass_index];
    if (!pass->program) return;

    glUseProgram(pass->program);

    GLint loc;

    /* Time uniforms */
    loc = glGetUniformLocation(pass->program, "iTime");
    if (loc >= 0) glUniform1f(loc, shader_time);

    loc = glGetUniformLocation(pass->program, "iTimeDelta");
    if (loc >= 0) glUniform1f(loc, 1.0f / 60.0f);  /* Approximate */

    loc = glGetUniformLocation(pass->program, "iFrameRate");
    if (loc >= 0) glUniform1f(loc, 60.0f);

    loc = glGetUniformLocation(pass->program, "iFrame");
    if (loc >= 0) glUniform1i(loc, shader->frame_count);

    /* Resolution */
    loc = glGetUniformLocation(pass->program, "iResolution");
    if (loc >= 0) {
        float w = (float)pass->width;
        float h = (float)pass->height;
        glUniform3f(loc, w, h, w / h);
    }

    /* Mouse */
    loc = glGetUniformLocation(pass->program, "iMouse");
    if (loc >= 0) {
        float click_x = mouse_click ? mouse_x : 0.0f;
        float click_y = mouse_click ? mouse_y : 0.0f;
        glUniform4f(loc, mouse_x, mouse_y, click_x, click_y);
    }

    /* Date */
    loc = glGetUniformLocation(pass->program, "iDate");
    if (loc >= 0) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        if (tm_info) {
            float year = (float)(tm_info->tm_year + 1900);
            float month = (float)(tm_info->tm_mon + 1);
            float day = (float)tm_info->tm_mday;
            float seconds = (float)(tm_info->tm_hour * 3600 +
                                    tm_info->tm_min * 60 +
                                    tm_info->tm_sec);
            glUniform4f(loc, year, month, day, seconds);
        }
    }

    loc = glGetUniformLocation(pass->program, "iSampleRate");
    if (loc >= 0) glUniform1f(loc, 44100.0f);

    /* Channel resolutions */
    loc = glGetUniformLocation(pass->program, "iChannelResolution");
    if (loc >= 0) {
        float resolutions[12] = {
            256.0f, 256.0f, 1.0f,
            256.0f, 256.0f, 1.0f,
            256.0f, 256.0f, 1.0f,
            256.0f, 256.0f, 1.0f
        };
        glUniform3fv(loc, 4, resolutions);
    }
}

void multipass_bind_textures(multipass_shader_t *shader, int pass_index) {
    if (!shader || pass_index < 0 || pass_index >= shader->pass_count) return;

    multipass_pass_t *pass = &shader->passes[pass_index];
    if (!pass->program) return;

    log_debug_frame(shader->frame_count, "Binding textures for pass %d (%s):", pass_index, pass->name);

    for (int c = 0; c < MULTIPASS_MAX_CHANNELS; c++) {
        glActiveTexture(GL_TEXTURE0 + c);

        GLuint tex = shader->noise_texture;  /* Default to noise */
        const char *source_name = "noise";
        (void)source_name; /* Used for debug logging */

        switch (pass->channels[c].source) {
            case CHANNEL_SOURCE_BUFFER_A:
            case CHANNEL_SOURCE_BUFFER_B:
            case CHANNEL_SOURCE_BUFFER_C:
            case CHANNEL_SOURCE_BUFFER_D: {
                int buf_idx = pass->channels[c].source - CHANNEL_SOURCE_BUFFER_A;
                multipass_pass_t *buf_pass = NULL;

                for (int i = 0; i < shader->pass_count; i++) {
                    if ((int)shader->passes[i].type == PASS_TYPE_BUFFER_A + buf_idx) {
                        buf_pass = &shader->passes[i];
                        break;
                    }
                }

                if (buf_pass && buf_pass->textures[0]) {
                    /*
                     * IMPORTANT: Read from the CURRENT ping-pong index
                     * This is the texture that was written to in the previous frame
                     * or the most recently completed render of this buffer
                     */
                    tex = buf_pass->textures[buf_pass->ping_pong_index];
                    source_name = buf_pass->name;
                    log_debug_frame(shader->frame_count, "  iChannel%d: Bound to %s tex[%d]=%u",
                              c, buf_pass->name, buf_pass->ping_pong_index, tex);
                } else {
                    log_debug_frame(shader->frame_count, "  iChannel%d: Buffer %c not found, using noise", c, 'A' + buf_idx);
                }
                break;
            }

            case CHANNEL_SOURCE_SELF:
                if (pass->textures[0]) {
                    /* For self-reference, read from current ping-pong (previous frame) */
                    tex = pass->textures[pass->ping_pong_index];
                    source_name = "self(feedback)";
                }
                break;

            case CHANNEL_SOURCE_NOISE:
            default:
                tex = shader->noise_texture;
                source_name = "noise";
                break;
        }

        glBindTexture(GL_TEXTURE_2D, tex);

        /* Set texture parameters for proper sampling */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        char name[16];
        snprintf(name, sizeof(name), "iChannel%d", c);
        GLint loc = glGetUniformLocation(pass->program, name);
        if (loc >= 0) {
            glUniform1i(loc, c);
        }
    }
}

void multipass_swap_buffers(multipass_shader_t *shader, int pass_index) {
    /*
     * NOTE: This function is now deprecated as ping-pong swapping
     * is handled directly in multipass_render_pass after rendering.
     * Kept for API compatibility.
     */
    (void)shader;
    (void)pass_index;
}

void multipass_render_pass(multipass_shader_t *shader,
                           int pass_index,
                           float time,
                           float mouse_x, float mouse_y,
                           bool mouse_click) {
    if (!shader || pass_index < 0 || pass_index >= shader->pass_count) return;

    multipass_pass_t *pass = &shader->passes[pass_index];

    if (!pass->is_compiled || !pass->program) {
        log_debug("Skipping pass %d (%s): not compiled", pass_index, pass->name);
        return;
    }

    log_debug_frame(shader->frame_count, "Rendering pass %d: %s (program=%u, fbo=%u, size=%dx%d)",
              pass_index, pass->name, pass->program, pass->fbo, pass->width, pass->height);

    /* Clear any stale GL errors before rendering */
    while (glGetError() != GL_NO_ERROR) {}

    /* Bind FBO for buffer passes, or default framebuffer for Image pass */
    if (pass->fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, pass->fbo);

        /*
         * Ping-pong buffer logic:
         * - ping_pong_index points to the texture containing the PREVIOUS frame's result
         * - We WRITE to the OTHER texture (1 - ping_pong_index)
         * - Other passes READ from ping_pong_index (previous result)
         * - After rendering, we swap so the newly written texture becomes readable
         */
        int write_idx = 1 - pass->ping_pong_index;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, pass->textures[write_idx], 0);

        log_debug_frame(shader->frame_count, "Pass %d: writing to tex[%d]=%u, reading from tex[%d]=%u",
                  pass_index, write_idx, pass->textures[write_idx],
                  pass->ping_pong_index, pass->textures[pass->ping_pong_index]);

        /* Check framebuffer status */
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            log_error("Framebuffer not complete for pass %d: 0x%x", pass_index, status);
            glBindFramebuffer(GL_FRAMEBUFFER, shader->default_framebuffer);
            return;
        }

        /* Clear on first frame */
        if (pass->needs_clear) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            pass->needs_clear = false;
        }
    } else {
        /* Image pass renders to screen - use the stored default framebuffer
         * (GTK GL contexts may use non-zero FBO as default) */
        glBindFramebuffer(GL_FRAMEBUFFER, shader->default_framebuffer);
    }

    glViewport(0, 0, pass->width, pass->height);

    /* Use program and set uniforms */
    glUseProgram(pass->program);
    multipass_set_uniforms(shader, pass_index, time, mouse_x, mouse_y, mouse_click);
    multipass_bind_textures(shader, pass_index);

    /* Draw fullscreen quad */
#if defined(HAVE_GLES3) || defined(USE_EPOXY)
    glBindVertexArray(shader->vao);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, shader->vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);

    /* For buffer passes, finalize the render */
    if (pass->fbo) {
        int write_idx = 1 - pass->ping_pong_index;

        /* Generate mipmaps for the texture we just rendered to (needed for textureLod) */
        glBindTexture(GL_TEXTURE_2D, pass->textures[write_idx]);
        glGenerateMipmap(GL_TEXTURE_2D);
        log_debug_frame(shader->frame_count, "Generated mipmaps for pass %d texture[%d]=%u",
                  pass_index, write_idx, pass->textures[write_idx]);

        /*
         * SWAP ping-pong index AFTER rendering:
         * Now ping_pong_index points to the texture we just wrote,
         * so other passes will read from our fresh output
         */
        pass->ping_pong_index = write_idx;
        log_debug_frame(shader->frame_count, "Pass %d: ping_pong_index now %d (points to freshly rendered texture)",
                  pass_index, pass->ping_pong_index);
    }

    /* Check for GL errors */
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        log_error("GL error after rendering pass %d (%s): 0x%x", pass_index, pass->name, err);
    }

    /* Unbind FBO only for buffer passes - restore to default */
    if (pass->fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, shader->default_framebuffer);
    }
}

void multipass_render(multipass_shader_t *shader,
                      float time,
                      float mouse_x, float mouse_y,
                      bool mouse_click) {
    if (!shader || !shader->is_initialized) return;

    /* Query the CURRENT framebuffer binding - this is GTK's rendering FBO.
     * GTK's GtkGLArea uses its own FBO, not framebuffer 0.
     * We must capture this at render time, not init time. */
    GLint current_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
    shader->default_framebuffer = current_fbo;

    log_debug_frame(shader->frame_count, "=== Frame %d ===", shader->frame_count);

    /*
     * Shadertoy rendering order:
     * 1. Render all buffer passes in order (A, B, C, D)
     * 2. After each buffer pass, generate mipmaps for textureLod support
     * 3. Render Image pass last to the screen
     */

    /* Render buffer passes first (in order A, B, C, D) */
    for (int type = PASS_TYPE_BUFFER_A; type <= PASS_TYPE_BUFFER_D; type++) {
        for (int i = 0; i < shader->pass_count; i++) {
            if ((int)shader->passes[i].type == type) {
                log_debug_frame(shader->frame_count, "Executing buffer pass: %s", shader->passes[i].name);
                multipass_render_pass(shader, i, time, mouse_x, mouse_y, mouse_click);
            }
        }
    }

    /* Render Image pass last (directly to screen) */
    if (shader->image_pass_index >= 0) {
        log_debug_frame(shader->frame_count, "Executing Image pass (index=%d)", shader->image_pass_index);

        /* Ensure we're rendering to the default framebuffer (screen) */
        /* GTK may use a non-zero FBO as the default, so use the stored value */
        glBindFramebuffer(GL_FRAMEBUFFER, shader->default_framebuffer);

        /* Clear any stale GL errors */
        while (glGetError() != GL_NO_ERROR) {}

        /* Get viewport size from Image pass */
        multipass_pass_t *image_pass = &shader->passes[shader->image_pass_index];
        glViewport(0, 0, image_pass->width, image_pass->height);

        /* Clear the screen before rendering Image pass */
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        multipass_render_pass(shader, shader->image_pass_index, time,
                              mouse_x, mouse_y, mouse_click);
    } else {
        log_error("No Image pass found! (image_pass_index=%d, pass_count=%d)",
                  shader->image_pass_index, shader->pass_count);
    }

    shader->frame_count++;
}

void multipass_reset(multipass_shader_t *shader) {
    if (!shader) return;

    shader->frame_count = 0;

    for (int i = 0; i < shader->pass_count; i++) {
        shader->passes[i].ping_pong_index = 0;
        shader->passes[i].needs_clear = true;
    }
}

/* ============================================
 * Query Functions
 * ============================================ */

const char *multipass_get_error(const multipass_shader_t *shader, int pass_index) {
    if (!shader || pass_index < 0 || pass_index >= shader->pass_count) {
        return NULL;
    }
    return shader->passes[pass_index].compile_error;
}

char *multipass_get_all_errors(const multipass_shader_t *shader) {
    if (!shader) return NULL;

    size_t total_len = 0;
    for (int i = 0; i < shader->pass_count; i++) {
        if (shader->passes[i].compile_error) {
            total_len += strlen(shader->passes[i].name) + 3;
            total_len += strlen(shader->passes[i].compile_error) + 2;
        }
    }

    if (total_len == 0) return NULL;

    char *result = malloc(total_len + 1);
    if (!result) return NULL;

    result[0] = '\0';
    for (int i = 0; i < shader->pass_count; i++) {
        if (shader->passes[i].compile_error) {
            strcat(result, shader->passes[i].name);
            strcat(result, ": ");
            strcat(result, shader->passes[i].compile_error);
            strcat(result, "\n");
        }
    }

    return result;
}

bool multipass_has_errors(const multipass_shader_t *shader) {
    if (!shader) return true;

    for (int i = 0; i < shader->pass_count; i++) {
        if (shader->passes[i].compile_error) {
            return true;
        }
    }

    return false;
}

bool multipass_is_ready(const multipass_shader_t *shader) {
    if (!shader || !shader->is_initialized) return false;

    for (int i = 0; i < shader->pass_count; i++) {
        if (!shader->passes[i].is_compiled) {
            return false;
        }
    }

    return true;
}

multipass_pass_t *multipass_get_pass_by_type(multipass_shader_t *shader,
                                              multipass_type_t type) {
    if (!shader) return NULL;

    for (int i = 0; i < shader->pass_count; i++) {
        if (shader->passes[i].type == type) {
            return &shader->passes[i];
        }
    }

    return NULL;
}

int multipass_get_pass_index(const multipass_shader_t *shader, multipass_type_t type) {
    if (!shader) return -1;

    for (int i = 0; i < shader->pass_count; i++) {
        if (shader->passes[i].type == type) {
            return i;
        }
    }

    return -1;
}

GLuint multipass_get_buffer_texture(const multipass_shader_t *shader,
                                     multipass_type_t type) {
    if (!shader) return 0;

    for (int i = 0; i < shader->pass_count; i++) {
        if (shader->passes[i].type == type) {
            return shader->passes[i].textures[shader->passes[i].ping_pong_index];
        }
    }

    return 0;
}

/* ============================================
 * Debug Functions
 * ============================================ */

void multipass_debug_dump(const multipass_shader_t *shader) {
    if (!shader) {
        log_debug("Multipass shader: NULL");
        return;
    }

    log_debug("=== Multipass Shader Debug ===");
    log_debug("Pass count: %d", shader->pass_count);
    log_debug("Image pass index: %d", shader->image_pass_index);
    log_debug("Has buffers: %d", shader->has_buffers);
    log_debug("Is initialized: %d", shader->is_initialized);
    log_debug("Frame count: %d", shader->frame_count);

    for (int i = 0; i < shader->pass_count; i++) {
        const multipass_pass_t *pass = &shader->passes[i];
        log_debug("--- Pass %d: %s ---", i, pass->name);
        log_debug("  Type: %d (%s)", pass->type, multipass_type_name(pass->type));
        log_debug("  Program: %u", pass->program);
        log_debug("  FBO: %u", pass->fbo);
        log_debug("  Textures: [%u, %u]", pass->textures[0], pass->textures[1]);
        log_debug("  Size: %dx%d", pass->width, pass->height);
        log_debug("  Compiled: %d", pass->is_compiled);
        log_debug("  Ping-pong: %d", pass->ping_pong_index);

        for (int c = 0; c < MULTIPASS_MAX_CHANNELS; c++) {
            log_debug("  Channel %d: %s", c,
                     multipass_channel_source_name(pass->channels[c].source));
        }

        if (pass->compile_error) {
            log_debug("  Error: %s", pass->compile_error);
        }
    }

    log_debug("=== End Multipass Debug ===");
}
