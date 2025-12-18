/* OpenGL Preview Component - Implementation
 * Handles the live shader preview rendering
 * Supports Shadertoy-style multipass rendering with BufferA-D
 */

#include "editor_preview.h"
#include "../shader_lib/shader_multipass.h"
#include "../shader_lib/shader_log.h"
#include "platform_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/* OpenGL headers included via platform_compat.h */

/* Module state */
static struct {
    GtkWidget *gl_area;
    GLuint vao;
    GLuint vbo;
    GLuint default_texture;
    bool gl_initialized;
    bool shader_valid;
    double start_time;
    bool paused;
    double pause_time;
    float time_speed;
    float mouse_x;
    float mouse_y;
    bool mouse_click;
    double current_fps;
    double last_fps_time;
    int frame_count;
    long long total_frame_count;
    double last_render_time;
    guint tick_callback_id;
    editor_preview_error_callback_t error_callback;
    gpointer error_callback_data;
    editor_preview_double_click_callback_t double_click_callback;
    gpointer double_click_callback_data;
    char *error_message;
    bool has_error;
    
    /* Multipass rendering (handles both single and multi-pass shaders) */
    multipass_shader_t *multipass_shader;
    char *current_shader_source;
} preview_state = {
    .gl_area = NULL,
    .vao = 0,
    .vbo = 0,
    .default_texture = 0,
    .gl_initialized = false,
    .shader_valid = false,
    .start_time = 0.0,
    .paused = false,
    .pause_time = 0.0,
    .time_speed = 1.0f,
    .mouse_x = 0.5f,
    .mouse_y = 0.5f,
    .mouse_click = false,
    .current_fps = 0.0,
    .last_fps_time = 0.0,
    .frame_count = 0,
    .total_frame_count = 0,
    .last_render_time = 0.0,
    .tick_callback_id = 0,
    .error_callback = NULL,
    .error_callback_data = NULL,
    .double_click_callback = NULL,
    .double_click_callback_data = NULL,
    .error_message = NULL,
    .has_error = false,
    .multipass_shader = NULL,
    .current_shader_source = NULL
};

/* Helper: Get current time in seconds */
static double get_time(void) {
    return platform_get_time();
}

/* Helper: Set error message */
static void set_error(const char *message) {
    /* Free old error message */
    if (preview_state.error_message) {
        g_free(preview_state.error_message);
    }

    /* Store complete error message without truncation */
    preview_state.error_message = g_strdup(message ? message : "Unknown error");
    preview_state.has_error = true;

    if (preview_state.error_callback) {
        preview_state.error_callback(preview_state.error_message, preview_state.error_callback_data);
    }
}

/* Helper: Clear error */
static void clear_error(void) {
    if (preview_state.error_message) {
        g_free(preview_state.error_message);
        preview_state.error_message = NULL;
    }
    preview_state.has_error = false;
}

/* Render tick callback - invalidates the GL area to trigger continuous rendering */
static gboolean render_tick_callback(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    (void)frame_clock;
    (void)user_data;

    /* Skip rendering if paused */
    if (preview_state.paused) {
        return G_SOURCE_CONTINUE;
    }

    /* Invalidate the GL area to trigger a render on this frame */
    gtk_gl_area_queue_render(GTK_GL_AREA(widget));

    return G_SOURCE_CONTINUE;
}



/* OpenGL realize callback - called when GL context is created */
static void on_gl_realize(GtkGLArea *area, gpointer user_data) {
    (void)user_data;

    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error(area) != NULL) {
        set_error("Failed to initialize OpenGL context");
        return;
    }

    /* Log OpenGL info for debugging performance issues */
    const char *gl_version = (const char *)glGetString(GL_VERSION);
    const char *gl_renderer = (const char *)glGetString(GL_RENDERER);
    const char *gl_vendor = (const char *)glGetString(GL_VENDOR);
    g_message("OpenGL Version: %s", gl_version ? gl_version : "unknown");
    g_message("OpenGL Renderer: %s", gl_renderer ? gl_renderer : "unknown");
    g_message("OpenGL Vendor: %s", gl_vendor ? gl_vendor : "unknown");

    /* Setup fullscreen quad vertices */
    static const float vertices[] = {
        -1.0f, -1.0f,  /* Bottom-left */
         1.0f, -1.0f,  /* Bottom-right */
        -1.0f,  1.0f,  /* Top-left */
         1.0f,  1.0f,  /* Top-right */
    };

#if defined(HAVE_GLES3) || defined(USE_EPOXY)
    glGenVertexArrays(1, &preview_state.vao);
    glBindVertexArray(preview_state.vao);
#endif

    glGenBuffers(1, &preview_state.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, preview_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* Generate default white noise texture */
    glGenTextures(1, &preview_state.default_texture);
    glBindTexture(GL_TEXTURE_2D, preview_state.default_texture);
    
    unsigned char *noise_data = malloc(256 * 256 * 4);
    if (noise_data) {
        for (int i = 0; i < 256 * 256 * 4; i++) {
            noise_data[i] = rand() % 255;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, noise_data);
        free(noise_data);
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    /* Multipass system handles its own FBO/texture resources */

    preview_state.gl_initialized = true;

    /* Initialize timing if not already done */
    if (preview_state.start_time == 0.0) {
        preview_state.start_time = get_time();
        preview_state.last_fps_time = preview_state.start_time;
        preview_state.frame_count = 0;
    }

    /* Add tick callback for continuous rendering now that widget is realized */
    if (preview_state.tick_callback_id == 0) {
        preview_state.tick_callback_id = gtk_widget_add_tick_callback(
            GTK_WIDGET(area),
            render_tick_callback,
            NULL,
            NULL
        );
    }

    clear_error();
}

/* OpenGL render callback - called every frame */
static gboolean on_gl_render(GtkGLArea *area, GdkGLContext *context, gpointer user_data) {
    (void)context;
    (void)user_data;

    /* If paused, don't update FPS or render */
    if (preview_state.paused) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return TRUE;
    }

    /* Update FPS counter (do this first, even if no shader) */
    preview_state.frame_count++;
    preview_state.total_frame_count++;
    double current = get_time();

    if (preview_state.last_render_time <= 0.001) {
        preview_state.last_render_time = current;
    }
    preview_state.last_render_time = current;

    double elapsed = current - preview_state.last_fps_time;

    /* Update FPS more frequently (every 0.1 seconds) for smoother display */
    if (elapsed >= 0.1) {
        preview_state.current_fps = preview_state.frame_count / elapsed;
        preview_state.frame_count = 0;
        preview_state.last_fps_time = current;
    }

    int width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
    int height = gtk_widget_get_allocated_height(GTK_WIDGET(area));

    /* Calculate shader time */
    double current_time;
    if (preview_state.paused) {
        current_time = preview_state.pause_time;
    } else {
        current_time = (get_time() - preview_state.start_time) * preview_state.time_speed;
    }

    /* ===== MULTIPASS RENDERING (handles both single and multi-pass shaders) ===== */
    if (preview_state.multipass_shader) {
        if (!preview_state.gl_initialized) {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            return TRUE;
        }
        
        /* Resize if needed */
        multipass_resize(preview_state.multipass_shader, width, height);
        
        /* Render all passes */
        float mouse_px = preview_state.mouse_x * width;
        float mouse_py = preview_state.mouse_y * height;
        
        multipass_render(preview_state.multipass_shader,
                        (float)current_time,
                        mouse_px, mouse_py,
                        preview_state.mouse_click);
        
        return TRUE;
    }

    /* No shader loaded - show default background */
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    return TRUE;
}

/* OpenGL unrealize callback - cleanup */
static void on_gl_unrealize(GtkGLArea *area, gpointer user_data) {
    (void)user_data;

    /* Skip if already cleaned up */
    if (!preview_state.gl_initialized && preview_state.vbo == 0 && preview_state.vao == 0) {
        return;
    }

    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error(area) != NULL) {
        return;
    }

    /* Remove tick callback */
    if (preview_state.tick_callback_id > 0) {
        gtk_widget_remove_tick_callback(GTK_WIDGET(area), preview_state.tick_callback_id);
        preview_state.tick_callback_id = 0;
    }

    /* Free OpenGL resources */
    /* Cleanup error message */
    if (preview_state.error_message) {
        g_free(preview_state.error_message);
        preview_state.error_message = NULL;
    }

    /* Cleanup multipass shader */
    if (preview_state.multipass_shader) {
        multipass_destroy(preview_state.multipass_shader);
        preview_state.multipass_shader = NULL;
    }

    if (preview_state.vbo != 0) {
        glDeleteBuffers(1, &preview_state.vbo);
        preview_state.vbo = 0;
    }

#if defined(HAVE_GLES3) || defined(USE_EPOXY)
    if (preview_state.vao != 0) {
        glDeleteVertexArrays(1, &preview_state.vao);
        preview_state.vao = 0;
    }
#endif

    preview_state.gl_initialized = false;
    preview_state.shader_valid = false;
}

/* Mouse motion callback */
static gboolean on_preview_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)user_data;

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    if (width > 0 && height > 0) {
        preview_state.mouse_x = (float)event->x / width;
        preview_state.mouse_y = 1.0f - ((float)event->y / height); /* Flip Y for OpenGL */
    }

    return FALSE;
}

/* Button press callback for double-click detection */
static gboolean on_preview_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    /* Detect double-click (left button) */
    if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
        if (preview_state.double_click_callback) {
            preview_state.double_click_callback(preview_state.double_click_callback_data);
        }
        return TRUE;
    }

    return FALSE;
}

/* Public API Implementation */

GtkWidget *editor_preview_create(void) {
    if (preview_state.gl_area) {
        g_warning("editor_preview_create: Preview already created");
        return preview_state.gl_area;
    }

    /* Create GL area widget */
    preview_state.gl_area = gtk_gl_area_new();
    
    /* Don't force OpenGL ES - let GTK use native desktop OpenGL for best performance
     * Desktop OpenGL 3.3+ is faster than ES compatibility mode on NVIDIA/AMD */
    gtk_gl_area_set_use_es(GTK_GL_AREA(preview_state.gl_area), FALSE);
    gtk_gl_area_set_required_version(GTK_GL_AREA(preview_state.gl_area), 3, 3);
    
    gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(preview_state.gl_area), FALSE);
    gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(preview_state.gl_area), FALSE);

    /* Disable auto-render, we'll use tick callback + queue_render for precise control */
    gtk_gl_area_set_auto_render(GTK_GL_AREA(preview_state.gl_area), FALSE);

    /* Connect OpenGL signals */
    g_signal_connect(preview_state.gl_area, "realize",
                     G_CALLBACK(on_gl_realize), NULL);
    g_signal_connect(preview_state.gl_area, "render",
                     G_CALLBACK(on_gl_render), NULL);
    g_signal_connect(preview_state.gl_area, "unrealize",
                     G_CALLBACK(on_gl_unrealize), NULL);

    /* Connect mouse motion for shader mouse input */
    gtk_widget_add_events(preview_state.gl_area, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
    g_signal_connect(preview_state.gl_area, "motion-notify-event",
                     G_CALLBACK(on_preview_motion), NULL);
    
    /* Connect button press for double-click detection */
    g_signal_connect(preview_state.gl_area, "button-press-event",
                     G_CALLBACK(on_preview_button_press), NULL);

    /* Initialize FPS timing */
    preview_state.start_time = get_time();
    preview_state.pause_time = preview_state.start_time;
    preview_state.total_frame_count = 0;
    preview_state.last_render_time = 0.0;
    preview_state.frame_count = 0;

    /* Tick callback will be added when widget is realized */

    return preview_state.gl_area;
}

bool editor_preview_compile_shader(const char *shader_code) {
    if (!shader_code) {
        set_error("No shader code provided");
        return false;
    }

    /* Check if GL context is initialized */
    if (!preview_state.gl_area) {
        set_error("GL area not created");
        return false;
    }

    /* Check if widget is realized */
    if (!gtk_widget_get_realized(preview_state.gl_area)) {
        set_error("GL context not yet realized");
        return false;
    }

    /* Make GL context current */
    gtk_gl_area_make_current(GTK_GL_AREA(preview_state.gl_area));

    /* Check for GL errors */
    GError *error = gtk_gl_area_get_error(GTK_GL_AREA(preview_state.gl_area));
    if (error != NULL) {
        char *full_error = g_strdup_printf("GL Context Error: %s", error->message);
        set_error(full_error);
        g_free(full_error);
        return false;
    }

    /* Store shader source for potential recompilation */
    if (preview_state.current_shader_source) {
        free(preview_state.current_shader_source);
    }
    preview_state.current_shader_source = strdup(shader_code);

    /* Clean up old multipass shader if exists */
    if (preview_state.multipass_shader) {
        multipass_destroy(preview_state.multipass_shader);
        preview_state.multipass_shader = NULL;
    }
    preview_state.shader_valid = false;

    /* All shaders go through multipass system (single-pass = Image-only multipass) */
    int main_count = multipass_count_main_functions(shader_code);
    log_info("Compiling shader with %d mainImage function(s)", main_count);
    
    /* Create multipass shader */
    preview_state.multipass_shader = multipass_create(shader_code);
    
    if (!preview_state.multipass_shader) {
        set_error("Failed to parse shader");
        return false;
    }
    
    int width = gtk_widget_get_allocated_width(preview_state.gl_area);
    int height = gtk_widget_get_allocated_height(preview_state.gl_area);
    
    /* Ensure minimum size */
    if (width < 16) width = 800;
    if (height < 16) height = 600;
    
    /* Initialize GL resources */
    if (!multipass_init_gl(preview_state.multipass_shader, width, height)) {
        set_error("Failed to initialize GL resources");
        multipass_destroy(preview_state.multipass_shader);
        preview_state.multipass_shader = NULL;
        return false;
    }
    
    /* Compile all passes */
    if (!multipass_compile_all(preview_state.multipass_shader)) {
        /* Compilation failed - get errors */
        char *errors = multipass_get_all_errors(preview_state.multipass_shader);
        GString *detailed_error = g_string_new("=== SHADER COMPILATION FAILED ===\n\n");
        
        if (errors) {
            g_string_append(detailed_error, errors);
            free(errors);
        } else {
            g_string_append(detailed_error, "Unknown compilation error\n");
        }
        
        set_error(detailed_error->str);
        g_string_free(detailed_error, TRUE);
        
        multipass_destroy(preview_state.multipass_shader);
        preview_state.multipass_shader = NULL;
        return false;
    }
    
    /* Success */
    preview_state.shader_valid = true;
    clear_error();
    
    log_info("Successfully compiled shader with %d pass(es)",
             preview_state.multipass_shader->pass_count);
    
    /* Debug dump */
    multipass_debug_dump(preview_state.multipass_shader);
    
    return true;
}

const char *editor_preview_get_error(void) {
    return preview_state.has_error ? preview_state.error_message : NULL;
}

bool editor_preview_has_shader(void) {
    return preview_state.shader_valid && 
           preview_state.multipass_shader && 
           multipass_is_ready(preview_state.multipass_shader);
}

void editor_preview_set_paused(bool paused) {
    if (paused && !preview_state.paused) {
        /* Pausing - store current time and reset FPS */
        preview_state.pause_time = (get_time() - preview_state.start_time) * preview_state.time_speed;
        preview_state.current_fps = 0.0;
        preview_state.frame_count = 0;
    } else if (!paused && preview_state.paused) {
        /* Resuming - adjust start time and reset FPS tracking */
        preview_state.start_time = get_time() - (preview_state.pause_time / preview_state.time_speed);
        preview_state.last_fps_time = get_time();
        preview_state.frame_count = 0;
    }

    preview_state.paused = paused;
}

bool editor_preview_is_paused(void) {
    return preview_state.paused;
}

void editor_preview_set_speed(float speed) {
    if (speed <= 0.0f) {
        speed = 1.0f;
    }
    preview_state.time_speed = speed;
}

float editor_preview_get_speed(void) {
    return preview_state.time_speed;
}

void editor_preview_reset_time(void) {
    preview_state.start_time = get_time();
    preview_state.total_frame_count = 0;
    
    /* Reset multipass shader buffers */
    if (preview_state.multipass_shader) {
        multipass_reset(preview_state.multipass_shader);
    }
}

double editor_preview_get_fps(void) {
    return preview_state.current_fps;
}

float editor_preview_get_resolution_scale(void) {
    if (preview_state.multipass_shader) {
        return multipass_get_resolution_scale(preview_state.multipass_shader);
    }
    return 1.0f;
}

void editor_preview_set_resolution_scale(float scale) {
    if (preview_state.multipass_shader) {
        /* Disable adaptive when manually setting scale */
        multipass_set_adaptive_resolution(preview_state.multipass_shader, false, 0, 0, 0);
        multipass_set_resolution_scale(preview_state.multipass_shader, scale);
    }
}

void editor_preview_set_adaptive_resolution(bool enabled) {
    if (preview_state.multipass_shader) {
        multipass_set_adaptive_resolution(preview_state.multipass_shader, 
                                          enabled, 
                                          55.0f,   /* target FPS */
                                          0.25f,   /* min scale */
                                          1.0f);   /* max scale */
    }
}

bool editor_preview_is_adaptive_resolution(void) {
    if (preview_state.multipass_shader) {
        return multipass_is_adaptive_resolution(preview_state.multipass_shader);
    }
    return false;
}

void editor_preview_get_mouse(float *x, float *y) {
    if (x) *x = preview_state.mouse_x;
    if (y) *y = preview_state.mouse_y;
}

void editor_preview_set_error_callback(editor_preview_error_callback_t callback,
                                       gpointer user_data) {
    preview_state.error_callback = callback;
    preview_state.error_callback_data = user_data;
}

void editor_preview_set_double_click_callback(editor_preview_double_click_callback_t callback,
                                               gpointer user_data) {
    preview_state.double_click_callback = callback;
    preview_state.double_click_callback_data = user_data;
}

void editor_preview_queue_render(void) {
    if (preview_state.gl_area) {
        gtk_widget_queue_draw(preview_state.gl_area);
    }
}

void editor_preview_destroy(void) {
    /* Remove tick callback if still active */
    if (preview_state.tick_callback_id > 0 && preview_state.gl_area) {
        gtk_widget_remove_tick_callback(preview_state.gl_area, preview_state.tick_callback_id);
        preview_state.tick_callback_id = 0;
    }

    /* Clean up OpenGL resources if context is still valid */
    if (preview_state.gl_area && gtk_widget_get_realized(preview_state.gl_area)) {
        gtk_gl_area_make_current(GTK_GL_AREA(preview_state.gl_area));
        
        if (gtk_gl_area_get_error(GTK_GL_AREA(preview_state.gl_area)) == NULL) {
            /* Cleanup multipass shader */
            if (preview_state.multipass_shader) {
                multipass_destroy(preview_state.multipass_shader);
                preview_state.multipass_shader = NULL;
            }

            if (preview_state.vbo != 0) {
                glDeleteBuffers(1, &preview_state.vbo);
                preview_state.vbo = 0;
            }

#if defined(HAVE_GLES3) || defined(USE_EPOXY)
            if (preview_state.vao != 0) {
                glDeleteVertexArrays(1, &preview_state.vao);
                preview_state.vao = 0;
            }
#endif
            
            if (preview_state.default_texture != 0) {
                glDeleteTextures(1, &preview_state.default_texture);
                preview_state.default_texture = 0;
            }
        }
    }

    /* Cleanup error message */
    if (preview_state.error_message) {
        g_free(preview_state.error_message);
        preview_state.error_message = NULL;
    }
    
    /* Cleanup shader source */
    if (preview_state.current_shader_source) {
        free(preview_state.current_shader_source);
        preview_state.current_shader_source = NULL;
    }

    /* Reset state */
    preview_state.gl_area = NULL;
    preview_state.error_callback = NULL;
    preview_state.error_callback_data = NULL;
    preview_state.gl_initialized = false;
    preview_state.shader_valid = false;
    preview_state.has_error = false;
}
