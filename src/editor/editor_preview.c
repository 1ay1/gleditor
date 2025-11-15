/* OpenGL Preview Component - Implementation
 * Handles the live shader preview rendering
 */

#include "editor_preview.h"
#include "../shader_lib/neowall_shader_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#ifdef HAVE_GLES3
#include <GLES3/gl3.h>
#else
#include <GLES2/gl2.h>
#endif

/* Module state */
static struct {
    GtkWidget *gl_area;
    GLuint shader_program;
    GLuint vao;
    GLuint vbo;
    bool gl_initialized;
    bool shader_valid;
    double start_time;
    bool paused;
    double pause_time;
    float time_speed;
    float mouse_x;
    float mouse_y;
    double current_fps;
    double last_fps_time;
    int frame_count;
    guint animation_timer_id;
    editor_preview_error_callback_t error_callback;
    gpointer error_callback_data;
    char *error_message;
    bool has_error;

    /* Optimization state */
    bool needs_render;
    double last_render_time;
    int target_fps;
    double min_frame_time;
    bool vsync_enabled;
    int cached_width;
    int cached_height;
    GLint cached_uniform_locations[10];
    bool uniforms_cached;
} preview_state = {
    .gl_area = NULL,
    .shader_program = 0,
    .vao = 0,
    .vbo = 0,
    .gl_initialized = false,
    .shader_valid = false,
    .start_time = 0.0,
    .paused = false,
    .pause_time = 0.0,
    .time_speed = 1.0f,
    .mouse_x = 0.5f,
    .mouse_y = 0.5f,
    .current_fps = 0.0,
    .last_fps_time = 0.0,
    .frame_count = 0,
    .animation_timer_id = 0,
    .error_callback = NULL,
    .error_callback_data = NULL,
    .error_message = NULL,
    .has_error = false,
    .needs_render = true,
    .last_render_time = 0.0,
    .target_fps = 60,
    .min_frame_time = 1.0 / 60.0,
    .vsync_enabled = true,
    .cached_width = 0,
    .cached_height = 0,
    .cached_uniform_locations = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    .uniforms_cached = false
};

/* Helper: Get current time in seconds */
static double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
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

/* Animation timer callback with frame rate limiting */
static gboolean animation_timer_cb(gpointer user_data) {
    (void)user_data;

    if (!preview_state.gl_area || !preview_state.shader_valid) {
        return G_SOURCE_CONTINUE;
    }

    /* Only render if not paused or if needed */
    if (!preview_state.paused || preview_state.needs_render) {
        double current_time = get_time();

        /* Frame rate limiting - only render if enough time has passed */
        if (current_time - preview_state.last_render_time >= preview_state.min_frame_time) {
            gtk_widget_queue_draw(preview_state.gl_area);
            preview_state.last_render_time = current_time;
            preview_state.needs_render = false;
        }
    }

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

    /* Enable VSync for smoother rendering and less tearing */
    if (preview_state.vsync_enabled) {
        gdk_gl_context_set_use_es(gtk_gl_area_get_context(area), TRUE);
    }

    /* Setup fullscreen quad vertices */
    static const float vertices[] = {
        -1.0f, -1.0f,  /* Bottom-left */
         1.0f, -1.0f,  /* Bottom-right */
        -1.0f,  1.0f,  /* Top-left */
         1.0f,  1.0f,  /* Top-right */
    };

#ifdef HAVE_GLES3
    glGenVertexArrays(1, &preview_state.vao);
    glBindVertexArray(preview_state.vao);
#endif

    glGenBuffers(1, &preview_state.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, preview_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* Optimize GL state */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DITHER);

    preview_state.gl_initialized = true;
    preview_state.start_time = get_time();
    preview_state.last_fps_time = preview_state.start_time;
    preview_state.last_render_time = preview_state.start_time;
    preview_state.frame_count = 0;
    preview_state.needs_render = true;

    clear_error();
}

/* OpenGL render callback - called every frame */
static gboolean on_gl_render(GtkGLArea *area, GdkGLContext *context, gpointer user_data) {
    (void)context;
    (void)user_data;

    /* Default background if no shader */
    if (!preview_state.gl_initialized || !preview_state.shader_valid ||
        preview_state.shader_program == 0) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return TRUE;
    }

    int width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
    int height = gtk_widget_get_allocated_height(GTK_WIDGET(area));

    /* Only update viewport if size changed */
    if (width != preview_state.cached_width || height != preview_state.cached_height) {
        glViewport(0, 0, width, height);
        preview_state.cached_width = width;
        preview_state.cached_height = height;
        preview_state.uniforms_cached = false; /* Recache uniforms on resize */
    }

    /* Calculate shader time */
    double current_time;
    if (preview_state.paused) {
        current_time = preview_state.pause_time;
    } else {
        current_time = (get_time() - preview_state.start_time) * preview_state.time_speed;
    }

    /* Use shader program */
    glUseProgram(preview_state.shader_program);

    /* Cache uniform locations on first render or after shader change */
    if (!preview_state.uniforms_cached) {
        preview_state.cached_uniform_locations[0] = glGetUniformLocation(preview_state.shader_program, "_neowall_time");
        preview_state.cached_uniform_locations[1] = glGetUniformLocation(preview_state.shader_program, "_neowall_resolution");
        preview_state.cached_uniform_locations[2] = glGetUniformLocation(preview_state.shader_program, "iResolution");
        preview_state.cached_uniform_locations[3] = glGetUniformLocation(preview_state.shader_program, "iTime");
        preview_state.cached_uniform_locations[4] = glGetUniformLocation(preview_state.shader_program, "iTimeDelta");
        preview_state.cached_uniform_locations[5] = glGetUniformLocation(preview_state.shader_program, "iFrame");
        preview_state.cached_uniform_locations[6] = glGetUniformLocation(preview_state.shader_program, "iMouse");
        preview_state.uniforms_cached = true;
    }

    /* Set uniforms using cached locations - much faster */
    if (preview_state.cached_uniform_locations[0] >= 0) {
        glUniform1f(preview_state.cached_uniform_locations[0], (float)current_time);
    }

    if (preview_state.cached_uniform_locations[1] >= 0) {
        glUniform2f(preview_state.cached_uniform_locations[1], (float)width, (float)height);
    }

    if (preview_state.cached_uniform_locations[2] >= 0) {
        float aspect = (width > 0 && height > 0) ? (float)width / (float)height : 1.0f;
        glUniform3f(preview_state.cached_uniform_locations[2], (float)width, (float)height, aspect);
    }

    if (preview_state.cached_uniform_locations[3] >= 0) {
        glUniform1f(preview_state.cached_uniform_locations[3], (float)current_time);
    }

    if (preview_state.cached_uniform_locations[4] >= 0) {
        glUniform1f(preview_state.cached_uniform_locations[4], preview_state.min_frame_time);
    }

    if (preview_state.cached_uniform_locations[5] >= 0) {
        glUniform1i(preview_state.cached_uniform_locations[5], preview_state.frame_count);
    }

    if (preview_state.cached_uniform_locations[6] >= 0) {
        glUniform4f(preview_state.cached_uniform_locations[6],
                    preview_state.mouse_x * width,
                    preview_state.mouse_y * height,
                    0.0f, 0.0f);
    }

    /* Draw fullscreen quad - VAO keeps state between frames */
#ifdef HAVE_GLES3
    glBindVertexArray(preview_state.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#else
    glBindBuffer(GL_ARRAY_BUFFER, preview_state.vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
#endif

    /* Update FPS counter */
    preview_state.frame_count++;
    double current = get_time();
    if (current - preview_state.last_fps_time >= 1.0) {
        preview_state.current_fps = preview_state.frame_count / (current - preview_state.last_fps_time);
        preview_state.frame_count = 0;
        preview_state.last_fps_time = current;
    }

    return TRUE;
}

/* OpenGL unrealize callback - cleanup */
static void on_gl_unrealize(GtkGLArea *area, gpointer user_data) {
    (void)user_data;

    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error(area) != NULL) {
        return;
    }

    /* Stop animation timer */
    if (preview_state.animation_timer_id) {
        g_source_remove(preview_state.animation_timer_id);
        preview_state.animation_timer_id = 0;
    }

    /* Free OpenGL resources */
    /* Cleanup error message */
    if (preview_state.error_message) {
        g_free(preview_state.error_message);
        preview_state.error_message = NULL;
    }

    /* Cleanup shader program */
    if (preview_state.shader_program) {
        glDeleteProgram(preview_state.shader_program);
        preview_state.shader_program = 0;
    }

    if (preview_state.vbo != 0) {
        glDeleteBuffers(1, &preview_state.vbo);
        preview_state.vbo = 0;
    }

#ifdef HAVE_GLES3
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

/* Public API Implementation */

GtkWidget *editor_preview_create(void) {
    if (preview_state.gl_area) {
        g_warning("editor_preview_create: Preview already created");
        return preview_state.gl_area;
    }

    /* Create GL area widget */
    preview_state.gl_area = gtk_gl_area_new();
    gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(preview_state.gl_area), FALSE);
    gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(preview_state.gl_area), FALSE);

    /* Connect OpenGL signals */
    g_signal_connect(preview_state.gl_area, "realize",
                     G_CALLBACK(on_gl_realize), NULL);
    g_signal_connect(preview_state.gl_area, "render",
                     G_CALLBACK(on_gl_render), NULL);
    g_signal_connect(preview_state.gl_area, "unrealize",
                     G_CALLBACK(on_gl_unrealize), NULL);

    /* Connect mouse motion for shader mouse input */
    gtk_widget_add_events(preview_state.gl_area, GDK_POINTER_MOTION_MASK);
    g_signal_connect(preview_state.gl_area, "motion-notify-event",
                     G_CALLBACK(on_preview_motion), NULL);

    /* Start animation timer - adaptive based on target FPS */
    int timer_interval = (int)(1000.0 / preview_state.target_fps);
    preview_state.animation_timer_id = g_timeout_add(timer_interval, animation_timer_cb, NULL);

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

    /* Delete old shader program */
    if (preview_state.shader_program != 0) {
        glDeleteProgram(preview_state.shader_program);
        preview_state.shader_program = 0;
        preview_state.shader_valid = false;
    }

    /* Compile shader using shader library */
    neowall_shader_result_t result = neowall_shader_compile(shader_code, NULL);

    if (!result.success) {
        /* Build detailed error message with all available information */
        GString *detailed_error = g_string_new("=== SHADER COMPILATION FAILED ===\n\n");

        if (result.error_message && strlen(result.error_message) > 0) {
            g_string_append(detailed_error, result.error_message);
        } else {
            g_string_append(detailed_error, "Compilation failed with no error message provided.\n");
        }

        /* Add shader source context if available */
        g_string_append(detailed_error, "\n\n=== SHADER SOURCE ===\n");
        g_string_append(detailed_error, "(Check console for full shader source with line numbers)\n");

        set_error(detailed_error->str);
        g_string_free(detailed_error, TRUE);
        neowall_shader_free_result(&result);
        return false;
    }

    /* Store new program */
    preview_state.shader_program = result.program;
    preview_state.shader_valid = true;
    preview_state.uniforms_cached = false; /* Force recache of uniform locations */
    preview_state.needs_render = true;

    clear_error();
    return true;
}

const char *editor_preview_get_error(void) {
    return preview_state.has_error ? preview_state.error_message : NULL;
}

bool editor_preview_has_shader(void) {
    return preview_state.shader_valid && preview_state.shader_program != 0;
}

void editor_preview_set_paused(bool paused) {
    if (paused && !preview_state.paused) {
        /* Pausing - store current time */
        preview_state.pause_time = (get_time() - preview_state.start_time) * preview_state.time_speed;
    } else if (!paused && preview_state.paused) {
        /* Resuming - adjust start time */
        preview_state.start_time = get_time() - (preview_state.pause_time / preview_state.time_speed);
        preview_state.needs_render = true;
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
    preview_state.needs_render = true;
}

float editor_preview_get_speed(void) {
    return preview_state.time_speed;
}

void editor_preview_reset_time(void) {
    preview_state.start_time = get_time();
    preview_state.pause_time = 0.0;
    preview_state.frame_count = 0;
    preview_state.needs_render = true;
}

double editor_preview_get_fps(void) {
    return preview_state.current_fps;
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

void editor_preview_queue_render(void) {
    preview_state.needs_render = true;
    if (preview_state.gl_area) {
        gtk_widget_queue_draw(preview_state.gl_area);
    }
}

void editor_preview_destroy(void) {
    /* Stop animation timer */
    if (preview_state.animation_timer_id) {
        g_source_remove(preview_state.animation_timer_id);
        preview_state.animation_timer_id = 0;
    }

    /* Cleanup is handled by GTK/OpenGL unrealize callback */
    preview_state.gl_area = NULL;
    preview_state.error_callback = NULL;
    preview_state.error_callback_data = NULL;
}
