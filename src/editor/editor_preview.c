/* OpenGL Preview Component - Implementation
 * Handles the live shader preview rendering
 */

#include "editor_preview.h"
#include "../shader_lib/neowall_shader_api.h"
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
    GLuint shader_program;      /* Pass 1: Image */
    GLuint buffer_program;      /* Pass 0: Buffer A */
    GLuint buffer_fbo;          /* FBO for Buffer A */
    GLuint buffer_tex[2];       /* Ping-pong textures for Buffer A */
    int buffer_width;
    int buffer_height;
    int ping_pong_index;
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
} preview_state = {
    .gl_area = NULL,
    .shader_program = 0,
    .buffer_program = 0,
    .buffer_fbo = 0,
    .buffer_tex = {0, 0},
    .buffer_width = 0,
    .buffer_height = 0,
    .ping_pong_index = 0,
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
    .has_error = false
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

    /* Setup Multipass FBO */
    glGenFramebuffers(1, &preview_state.buffer_fbo);
    glGenTextures(2, preview_state.buffer_tex);
    
    /* Initialize ping-pong textures */
    preview_state.buffer_width = 800; /* Default size */
    preview_state.buffer_height = 450;
    
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, preview_state.buffer_tex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, preview_state.buffer_width, preview_state.buffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

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
    double dt = current - preview_state.last_render_time;
    preview_state.last_render_time = current;

    double elapsed = current - preview_state.last_fps_time;

    /* Update FPS more frequently (every 0.1 seconds) for smoother display */
    if (elapsed >= 0.1) {
        preview_state.current_fps = preview_state.frame_count / elapsed;
        preview_state.frame_count = 0;
        preview_state.last_fps_time = current;
    }

    /* Default background if no shader */
    if (!preview_state.gl_initialized || !preview_state.shader_valid ||
        preview_state.shader_program == 0) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return TRUE;
    }

    int width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
    int height = gtk_widget_get_allocated_height(GTK_WIDGET(area));

    glViewport(0, 0, width, height);

    /* Calculate shader time */
    double current_time;
    if (preview_state.paused) {
        current_time = preview_state.pause_time;
    } else {
        current_time = (get_time() - preview_state.start_time) * preview_state.time_speed;
    }

    /* Common Uniform Setup Helper */
    GLint loc;
    #define SET_UNIFORMS(prog) \
        do { \
            loc = glGetUniformLocation(prog, "_neowall_time"); if (loc >= 0) glUniform1f(loc, (float)current_time); \
            loc = glGetUniformLocation(prog, "_neowall_resolution"); if (loc >= 0) glUniform2f(loc, (float)width, (float)height); \
            loc = glGetUniformLocation(prog, "_neowall_mouse"); if (loc >= 0) glUniform4f(loc, preview_state.mouse_x * width, preview_state.mouse_y * height, 0.0f, 0.0f); \
            loc = glGetUniformLocation(prog, "_neowall_frame"); if (loc >= 0) glUniform1i(loc, (int)preview_state.total_frame_count); \
            loc = glGetUniformLocation(prog, "iResolution"); if (loc >= 0) { float aspect = (width > 0 && height > 0) ? (float)width / (float)height : 1.0f; glUniform3f(loc, (float)width, (float)height, aspect); } \
            loc = glGetUniformLocation(prog, "iTime"); if (loc >= 0) glUniform1f(loc, (float)current_time); \
            loc = glGetUniformLocation(prog, "iTimeDelta"); if (loc >= 0) glUniform1f(loc, (float)dt); \
            loc = glGetUniformLocation(prog, "iFrameRate"); if (loc >= 0) glUniform1f(loc, (float)preview_state.current_fps); \
            loc = glGetUniformLocation(prog, "iFrame"); if (loc >= 0) glUniform1i(loc, (int)preview_state.total_frame_count); \
            loc = glGetUniformLocation(prog, "iMouse"); if (loc >= 0) glUniform4f(loc, preview_state.mouse_x * width, preview_state.mouse_y * height, 0.0f, 0.0f); \
            loc = glGetUniformLocation(prog, "_neowall_date"); \
            if (loc >= 0) { \
                float year=2024, month=1, day=1, seconds=0; \
                /* Use time(NULL) for simplicity or platform specific */ \
                time_t t = time(NULL); \
                struct tm *tm_info = localtime(&t); \
                if(tm_info) { \
                    year = (float)(tm_info->tm_year + 1900); month = (float)(tm_info->tm_mon + 1); day = (float)tm_info->tm_mday; \
                    seconds = (float)(tm_info->tm_hour * 3600 + tm_info->tm_min * 60 + tm_info->tm_sec); \
                } \
                glUniform4f(loc, year, month, day, seconds); \
            } \
        } while(0)

    /* PASS 0: Buffer A (if available) */
    if (preview_state.buffer_program) {
        /* Resize buffer textures if needed */
        if (preview_state.buffer_width != width || preview_state.buffer_height != height) {
            preview_state.buffer_width = width;
            preview_state.buffer_height = height;
            for (int i = 0; i < 2; i++) {
                glBindTexture(GL_TEXTURE_2D, preview_state.buffer_tex[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            }
        }

        /* Bind FBO and set write target to next ping-pong texture */
        int write_idx = (preview_state.ping_pong_index + 1) % 2;
        int read_idx = preview_state.ping_pong_index;
        
        glBindFramebuffer(GL_FRAMEBUFFER, preview_state.buffer_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, preview_state.buffer_tex[write_idx], 0);
        glViewport(0, 0, width, height);
        
        glUseProgram(preview_state.buffer_program);
        SET_UNIFORMS(preview_state.buffer_program);
        
        /* Bind Inputs for Buffer A */
        /* iChannel0 = Default Noise (Common for procedural shaders) */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, preview_state.default_texture);
        /* iChannel1 = Previous Frame (Feedback) */
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, preview_state.buffer_tex[read_idx]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, preview_state.default_texture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, preview_state.default_texture);
        
        /* Set samplers */
        for (int i = 0; i < 4; i++) {
            char name[16];
            snprintf(name, sizeof(name), "iChannel%d", i);
            loc = glGetUniformLocation(preview_state.buffer_program, name);
            if (loc >= 0) glUniform1i(loc, i);
        }
        
        /* Draw Pass 0 */
#if defined(HAVE_GLES3) || defined(USE_EPOXY)
        glBindVertexArray(preview_state.vao);
#endif
        glBindBuffer(GL_ARRAY_BUFFER, preview_state.vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        /* Swap ping-pong index */
        preview_state.ping_pong_index = write_idx;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    /* PASS 1: Image (Screen) */
    glViewport(0, 0, width, height);
    glUseProgram(preview_state.shader_program);
    SET_UNIFORMS(preview_state.shader_program);

    /* Bind Inputs for Image Pass */
    /* If multipass, iChannel0 = Buffer A Output. Else Noise. */
    glActiveTexture(GL_TEXTURE0);
    if (preview_state.buffer_program) {
        glBindTexture(GL_TEXTURE_2D, preview_state.buffer_tex[preview_state.ping_pong_index]);
    } else {
        glBindTexture(GL_TEXTURE_2D, preview_state.default_texture);
    }
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, preview_state.default_texture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, preview_state.default_texture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, preview_state.default_texture);

    /* Set samplers */
    for (int i = 0; i < 4; i++) {
        char name[16];
        snprintf(name, sizeof(name), "iChannel%d", i);
        loc = glGetUniformLocation(preview_state.shader_program, name);
        if (loc >= 0) glUniform1i(loc, i);
    }

    /* Draw Pass 1 */
#if defined(HAVE_GLES3) || defined(USE_EPOXY)
    glBindVertexArray(preview_state.vao);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, preview_state.vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);

    return TRUE;
}

/* OpenGL unrealize callback - cleanup */
static void on_gl_unrealize(GtkGLArea *area, gpointer user_data) {
    (void)user_data;

    /* Skip if already cleaned up */
    if (!preview_state.gl_initialized && preview_state.shader_program == 0 && 
        preview_state.vbo == 0 && preview_state.vao == 0) {
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

    /* Cleanup shader program */
    if (preview_state.shader_program != 0) {
        glDeleteProgram(preview_state.shader_program);
        preview_state.shader_program = 0;
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

    /* Delete old shader programs */
    if (preview_state.shader_program != 0) {
        glDeleteProgram(preview_state.shader_program);
        preview_state.shader_program = 0;
    }
    if (preview_state.buffer_program != 0) {
        glDeleteProgram(preview_state.buffer_program);
        preview_state.buffer_program = 0;
    }
    preview_state.shader_valid = false;

    /* Compile shader using shader library */
    neowall_shader_options_t options = NEOWALL_SHADER_OPTIONS_DEFAULT;
    options.use_es3 = true; /* Force ES3 for modern shaders */
    
    /* Try to compile Pass 0 (Buffer A or Single Pass) */
    options.pass_index = 0;
    neowall_shader_result_t res0 = neowall_shader_compile(shader_code, &options);
    
    /* Force Single Pass Mode: Only use Pass 0 (Buffer A / Main Scene) */
    /* This avoids black screen issues with multipass composition */
    
    if (res0.success) {
        preview_state.buffer_program = 0;
        preview_state.shader_program = res0.program;
        preview_state.shader_valid = true;
        clear_error();
    } else {
        /* Failure */
        GString *detailed_error = g_string_new("=== SHADER COMPILATION FAILED ===\n\n");

        if (res0.error_message && strlen(res0.error_message) > 0) {
            g_string_append(detailed_error, res0.error_message);
        } else {
            g_string_append(detailed_error, "Compilation failed with no error message provided.\n");
        }

        /* Add shader source context if available */
        g_string_append(detailed_error, "\n\n=== SHADER SOURCE ===\n");
        g_string_append(detailed_error, "(Check console for full shader source with line numbers)\n");

        set_error(detailed_error->str);
        g_string_free(detailed_error, TRUE);
        neowall_shader_free_result(&res0);
        return false;
    }

    neowall_shader_free_result(&res0); // Safe, only frees strings
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
    preview_state.pause_time = 0.0;
    preview_state.frame_count = 0;
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
            /* Cleanup shader program */
            if (preview_state.shader_program) {
                glDeleteProgram(preview_state.shader_program);
                preview_state.shader_program = 0;
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
        }
    }

    /* Cleanup error message */
    if (preview_state.error_message) {
        g_free(preview_state.error_message);
        preview_state.error_message = NULL;
    }

    /* Reset state */
    preview_state.gl_area = NULL;
    preview_state.error_callback = NULL;
    preview_state.error_callback_data = NULL;
    preview_state.gl_initialized = false;
    preview_state.shader_valid = false;
    preview_state.has_error = false;
}
