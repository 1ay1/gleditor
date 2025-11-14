/* NeoWall Shader Editor - Standalone Implementation
 * Live GLSL shader editor with real-time preview and NeoWall installation
 */

#include "shader_editor.h"
#include "shader_lib/neowall_shader_api.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

/* Suppress GTimeVal deprecation warnings from GtkSourceView headers */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop

#ifdef HAVE_GLES3
#include <GLES3/gl3.h>
#else
#include <GLES2/gl2.h>
#endif

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

/* UI Layout Constants */
#define EDITOR_TOOLBAR_SPACING 8
#define EDITOR_TOOLBAR_MARGIN 12
#define BUTTON_MARGIN 4
#define SEPARATOR_SPACING 8
#define PANED_MARGIN 8
#define STATUSBAR_SPACING 12
#define STATUSBAR_MARGIN_H 12
#define STATUSBAR_MARGIN_V 6
#define DIALOG_WIDTH_STANDARD 1400
#define DIALOG_HEIGHT_STANDARD 850

/* NeoWall shader directory */
#define NEOWALL_SHADER_DIR ".config/neowall/shaders"

/* Window state */
static GtkWidget *editor_window = NULL;
static GtkSourceBuffer *source_buffer = NULL;
static GtkWidget *gl_area = NULL;
static GtkWidget *error_label = NULL;
static GtkWidget *status_label = NULL;
static GtkWidget *fps_label = NULL;
static GtkWidget *source_view = NULL;
static GtkWidget *cursor_label = NULL;
static GtkWidget *compile_btn = NULL;
static char *current_file_path = NULL;

/* Editor settings */
static int editor_tab_width = 4;
static bool editor_auto_compile = true;
static int preview_fps = 60;

/* Shader rendering settings */
static float shader_time_speed = 1.0f;
static bool shader_paused = false;
static double shader_pause_time = 0.0;
static float shader_mouse_x = 0.5f;
static float shader_mouse_y = 0.5f;

/* OpenGL state */
static GLuint shader_program = 0;
static GLuint vao = 0;
static GLuint vbo = 0;
static bool gl_initialized = false;
static bool shader_valid = false;
static double start_time = 0.0;

/* Animation timer ID */
static guint animation_timer_id = 0;

/* Compile debounce timer */
static guint compile_timeout_id = 0;

/* FPS tracking */
static double last_fps_update_time = 0.0;
static int frame_count = 0;
static double current_fps = 0.0;

/* Default shader template */
static const char *default_shader =
"// NeoWall Shader - Shadertoy Compatible\n"
"// Created with NeoWall Shader Editor\n"
"\n"
"void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
"    // Normalized pixel coordinates (from 0 to 1)\n"
"    vec2 uv = fragCoord / iResolution.xy;\n"
"    \n"
"    // Time varying pixel color\n"
"    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));\n"
"    \n"
"    // Output to screen\n"
"    fragColor = vec4(col, 1.0);\n"
"}\n";

/* Forward declarations */
static gboolean animation_timer_cb(gpointer user_data);

/* Get current time in seconds */
static double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/* Get NeoWall shader directory path */
static char *get_neowall_shader_dir(void) {
    const char *home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    char *path = g_strdup_printf("%s/%s", home, NEOWALL_SHADER_DIR);

    /* Create directory if it doesn't exist */
    char *dir_copy = g_strdup(path);
    char *p = dir_copy;

    /* Skip home directory */
    p = strchr(p + 1, '/');
    while (p) {
        *p = '\0';
        mkdir(dir_copy, 0755);
        *p = '/';
        p = strchr(p + 1, '/');
    }
    mkdir(dir_copy, 0755);

    g_free(dir_copy);
    return path;
}

/* Update status bar */
static void update_status(const char *message) {
    if (status_label) {
        char markup[512];
        snprintf(markup, sizeof(markup), "<span foreground='#00FF41'>%s</span>", message);
        gtk_label_set_markup(GTK_LABEL(status_label), markup);
    }
}

/* Update FPS display */
static void update_fps_display(void) {
    if (!fps_label) return;

    double now = get_time();
    frame_count++;

    if (now - last_fps_update_time >= 0.5) {
        current_fps = frame_count / (now - last_fps_update_time);
        frame_count = 0;
        last_fps_update_time = now;

        char fps_text[128];
        snprintf(fps_text, sizeof(fps_text),
                "<span foreground='#00FF41'><b>FPS: %.1f</b></span>", current_fps);
        gtk_label_set_markup(GTK_LABEL(fps_label), fps_text);
    }
}

/* Update cursor position display */
static void on_cursor_moved(GtkTextBuffer *buffer, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    (void)user_data;

    if (!cursor_label) return;

    GtkTextIter iter;
    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

    int line = gtk_text_iter_get_line(&iter) + 1;
    int col = gtk_text_iter_get_line_offset(&iter) + 1;

    char text[128];
    snprintf(text, sizeof(text), "<span size='small'>📍 Line %d, Col %d</span>", line, col);
    gtk_label_set_markup(GTK_LABEL(cursor_label), text);
}

/* Compile and update shader */
static void update_shader_program(void) {
    if (!gl_initialized) return;

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(source_buffer), &start, &end);
    char *shader_source = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(source_buffer),
                                                    &start, &end, FALSE);

    /* Destroy old program */
    if (shader_program != 0) {
        glDeleteProgram(shader_program);
        shader_program = 0;
    }

    /* Compile new shader */
    neowall_shader_options_t options = NEOWALL_SHADER_OPTIONS_DEFAULT;
#ifdef HAVE_GLES3
    options.use_es3 = true;
#endif
    options.verbose_errors = true;

    neowall_shader_result_t result = neowall_shader_compile(shader_source, &options);

    if (result.success) {
        shader_program = result.program;
        shader_valid = true;

        gtk_label_set_markup(GTK_LABEL(error_label),
                           "<span foreground='green'>✓ Shader compiled successfully</span>");
        update_status("Ready - Shader compiled successfully");

        /* Start animation timer if not already running */
        if (!animation_timer_id && gl_area) {
            int interval = 1000 / preview_fps;
            animation_timer_id = g_timeout_add(interval, animation_timer_cb, gl_area);
            printf("[ShaderEditor] Started animation timer: %dms interval (%d FPS), timer_id=%u\n",
                   interval, preview_fps, animation_timer_id);
        }
    } else {
        shader_valid = false;

        char error_msg[1024];
        if (result.error_line >= 0) {
            snprintf(error_msg, sizeof(error_msg),
                    "<span foreground='red'>✗ Error at line %d: %s</span>",
                    result.error_line, result.error_message);
        } else {
            snprintf(error_msg, sizeof(error_msg),
                    "<span foreground='red'>✗ Compilation error: %s</span>",
                    result.error_message);
        }
        gtk_label_set_markup(GTK_LABEL(error_label), error_msg);
        update_status("Error - Check error message above");
    }

    neowall_shader_free_result(&result);
    g_free(shader_source);

    /* Trigger initial redraw */
    if (gl_area) {
        gtk_widget_queue_draw(gl_area);
    }
}

/* Compile timeout callback */
static gboolean compile_timeout_cb(gpointer user_data) {
    (void)user_data;
    compile_timeout_id = 0;
    update_shader_program();
    return G_SOURCE_REMOVE;
}

/* Buffer changed callback */
static void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
    (void)buffer;
    (void)user_data;

    if (!editor_auto_compile) return;

    /* Debounce compilation */
    if (compile_timeout_id != 0) {
        g_source_remove(compile_timeout_id);
    }

    compile_timeout_id = g_timeout_add(500, compile_timeout_cb, NULL);
}

/* OpenGL realize callback */
static void on_gl_realize(GtkGLArea *area, gpointer user_data) {
    (void)user_data;

    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error(area) != NULL) {
        fprintf(stderr, "Failed to initialize OpenGL\n");
        return;
    }

    /* Setup fullscreen quad */
    static const float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };

#ifdef HAVE_GLES3
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#endif

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    gl_initialized = true;
    start_time = get_time();
    last_fps_update_time = start_time;
    frame_count = 0;

    /* Initial shader compilation */
    update_shader_program();
}

/* Animation timer callback */
static gboolean animation_timer_cb(gpointer user_data) {
    GtkWidget *area = GTK_WIDGET(user_data);
    gtk_widget_queue_draw(area);
    return G_SOURCE_CONTINUE; /* Keep running */
}

/* OpenGL render callback */
static gboolean on_gl_render(GtkGLArea *area, GdkGLContext *context, gpointer user_data) {
    (void)context;
    (void)user_data;

    if (!gl_initialized || !shader_valid || shader_program == 0) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        printf("Render: not ready (gl_init=%d, valid=%d, prog=%u)\n", gl_initialized, shader_valid, shader_program);
        return TRUE;
    }

    int width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
    int height = gtk_widget_get_allocated_height(GTK_WIDGET(area));

    glViewport(0, 0, width, height);

    /* Calculate time */
    double current_time;
    if (shader_paused) {
        current_time = shader_pause_time;
    } else {
        current_time = (get_time() - start_time) * shader_time_speed;
    }

    static int render_count = 0;
    if (render_count % 60 == 0) {
        printf("Rendering frame %d, time=%.2f, paused=%d\n", render_count, current_time, shader_paused);
    }
    render_count++;

    /* Use shader program */
    glUseProgram(shader_program);

    /* Set uniforms - use NeoWall internal uniform names */
    GLint loc;

    /* Primary uniforms that the shader wrapper expects */
    loc = glGetUniformLocation(shader_program, "_neowall_time");
    if (loc >= 0) glUniform1f(loc, (float)current_time);

    loc = glGetUniformLocation(shader_program, "_neowall_resolution");
    if (loc >= 0) glUniform2f(loc, (float)width, (float)height);

    /* Also set Shadertoy uniforms if they exist (for compatibility) */
    loc = glGetUniformLocation(shader_program, "iResolution");
    if (loc >= 0) glUniform3f(loc, (float)width, (float)height, 1.0f);

    loc = glGetUniformLocation(shader_program, "iTime");
    if (loc >= 0) glUniform1f(loc, (float)current_time);

    loc = glGetUniformLocation(shader_program, "iTimeDelta");
    if (loc >= 0) glUniform1f(loc, 1.0f / preview_fps);

    loc = glGetUniformLocation(shader_program, "iFrame");
    if (loc >= 0) glUniform1i(loc, frame_count);

    loc = glGetUniformLocation(shader_program, "iMouse");
    if (loc >= 0) glUniform4f(loc, shader_mouse_x * width, shader_mouse_y * height, 0.0f, 0.0f);

    /* Draw fullscreen quad */
#ifdef HAVE_GLES3
    glBindVertexArray(vao);
#endif

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);

    update_fps_display();

    return TRUE;
}

/* OpenGL unrealize callback */
static void on_gl_unrealize(GtkGLArea *area, gpointer user_data) {
    (void)user_data;

    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error(area) != NULL) {
        return;
    }

    /* Stop animation timer */
    if (animation_timer_id) {
        g_source_remove(animation_timer_id);
        animation_timer_id = 0;
    }

    if (shader_program != 0) {
        glDeleteProgram(shader_program);
        shader_program = 0;
    }

    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }

#ifdef HAVE_GLES3
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
#endif

    gl_initialized = false;
}

/* Save shader to file */
static void on_save_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Shader",
                                                     GTK_WINDOW(editor_window),
                                                     GTK_FILE_CHOOSER_ACTION_SAVE,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Save", GTK_RESPONSE_ACCEPT,
                                                     NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (current_file_path) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), current_file_path);
    } else {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "shader.glsl");
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(source_buffer), &start, &end);
        char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(source_buffer), &start, &end, FALSE);

        GError *error = NULL;
        if (g_file_set_contents(filename, text, -1, &error)) {
            g_free(current_file_path);
            current_file_path = filename;
            update_status("Shader saved successfully");
        } else {
            char msg[512];
            snprintf(msg, sizeof(msg), "Failed to save: %s", error->message);
            update_status(msg);
            g_error_free(error);
            g_free(filename);
        }

        g_free(text);
    }

    gtk_widget_destroy(dialog);
}

/* Load shader from file */
static void on_load_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Load Shader",
                                                     GTK_WINDOW(editor_window),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Open", GTK_RESPONSE_ACCEPT,
                                                     NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        char *contents = NULL;
        GError *error = NULL;

        if (g_file_get_contents(filename, &contents, NULL, &error)) {
            gtk_text_buffer_set_text(GTK_TEXT_BUFFER(source_buffer), contents, -1);
            g_free(current_file_path);
            current_file_path = filename;
            update_status("Shader loaded successfully");
            g_free(contents);
        } else {
            char msg[512];
            snprintf(msg, sizeof(msg), "Failed to load: %s", error->message);
            update_status(msg);
            g_error_free(error);
            g_free(filename);
        }
    }

    gtk_widget_destroy(dialog);
}

/* Install shader to NeoWall */
static void on_install_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    /* Ask for shader name */
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Install to NeoWall",
                                                     GTK_WINDOW(editor_window),
                                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Install", GTK_RESPONSE_ACCEPT,
                                                     NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new("Shader name:");
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "my_shader.glsl");

    gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 5);
    gtk_widget_show_all(content);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *shader_name = gtk_entry_get_text(GTK_ENTRY(entry));

        if (shader_name && strlen(shader_name) > 0) {
            char *shader_dir = get_neowall_shader_dir();
            if (shader_dir) {
                char *install_path = g_strdup_printf("%s/%s", shader_dir, shader_name);

                GtkTextIter start, end;
                gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(source_buffer), &start, &end);
                char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(source_buffer), &start, &end, FALSE);

                GError *error = NULL;
                if (g_file_set_contents(install_path, text, -1, &error)) {
                    char msg[512];
                    snprintf(msg, sizeof(msg), "Shader installed to: %s", install_path);
                    update_status(msg);
                } else {
                    char msg[512];
                    snprintf(msg, sizeof(msg), "Installation failed: %s", error->message);
                    update_status(msg);
                    g_error_free(error);
                }

                g_free(text);
                g_free(install_path);
                g_free(shader_dir);
            } else {
                update_status("Failed to get NeoWall shader directory");
            }
        }
    }

    gtk_widget_destroy(dialog);
}

/* Reset to default shader */
static void on_reset_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(source_buffer), default_shader, -1);
    update_status("Reset to default shader");
}

/* Compile button clicked */
static void on_compile_clicked(GtkWidget *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    update_shader_program();
}

/* Pause/Resume toggle */
static void on_pause_toggled(GtkToggleButton *button, gpointer user_data) {
    (void)user_data;

    shader_paused = gtk_toggle_button_get_active(button);
    if (shader_paused) {
        shader_pause_time = get_time() - start_time;
    }
}

/* Mouse motion in preview */
static gboolean on_preview_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)user_data;

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    shader_mouse_x = event->x / width;
    shader_mouse_y = 1.0f - (event->y / height);

    return FALSE;
}

/* Window destroy callback */
static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;

    if (animation_timer_id) {
        g_source_remove(animation_timer_id);
        animation_timer_id = 0;
    }

    if (compile_timeout_id != 0) {
        g_source_remove(compile_timeout_id);
        compile_timeout_id = 0;
    }

    g_free(current_file_path);
    current_file_path = NULL;

    editor_window = NULL;
    source_buffer = NULL;
    gl_area = NULL;
}

/* Show shader editor window */
void shader_editor_show(GtkApplication *app) {
    if (editor_window != NULL) {
        gtk_window_present(GTK_WINDOW(editor_window));
        return;
    }

    /* Create main window */
    editor_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(editor_window), "🎨 gleditor");
    gtk_window_set_default_size(GTK_WINDOW(editor_window),
                               DIALOG_WIDTH_STANDARD, DIALOG_HEIGHT_STANDARD);
    gtk_container_set_border_width(GTK_CONTAINER(editor_window), 0);
    gtk_window_set_position(GTK_WINDOW(editor_window), GTK_WIN_POS_CENTER);

    /* Set window icon */
    gtk_window_set_icon_name(GTK_WINDOW(editor_window), "applications-graphics");

    /* Main vertical box */
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(editor_window), main_box);

    /* Toolbar with styling */
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, EDITOR_TOOLBAR_SPACING);
    gtk_widget_set_margin_start(toolbar, EDITOR_TOOLBAR_MARGIN);
    gtk_widget_set_margin_end(toolbar, EDITOR_TOOLBAR_MARGIN);
    gtk_widget_set_margin_top(toolbar, EDITOR_TOOLBAR_MARGIN);
    gtk_widget_set_margin_bottom(toolbar, EDITOR_TOOLBAR_MARGIN);

    /* Add background to toolbar */
    GtkCssProvider *toolbar_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(toolbar_css,
        "box { background-color: #2d2d2d; padding: 8px; border-radius: 6px; }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(toolbar),
        GTK_STYLE_PROVIDER(toolbar_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(toolbar_css);

    gtk_box_pack_start(GTK_BOX(main_box), toolbar, FALSE, FALSE, 0);

    /* Buttons with icons */
    GtkWidget *new_btn = gtk_button_new_with_label("🆕 New");
    GtkWidget *load_btn = gtk_button_new_with_label("📂 Load");
    GtkWidget *save_btn = gtk_button_new_with_label("💾 Save");
    GtkWidget *install_btn = gtk_button_new_with_label("📦 Install to NeoWall");
    compile_btn = gtk_button_new_with_label("⚡ Compile");
    GtkWidget *pause_btn = gtk_toggle_button_new_with_label("⏸ Pause");

    /* Style install button as suggested action */
    GtkStyleContext *install_ctx = gtk_widget_get_style_context(install_btn);
    gtk_style_context_add_class(install_ctx, "suggested-action");

    gtk_box_pack_start(GTK_BOX(toolbar), new_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), load_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), save_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, SEPARATOR_SPACING);
    gtk_box_pack_start(GTK_BOX(toolbar), compile_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), pause_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, SEPARATOR_SPACING);
    gtk_box_pack_start(GTK_BOX(toolbar), install_btn, FALSE, FALSE, 0);

    /* Spacer */
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), spacer, TRUE, TRUE, 0);

    /* FPS label with matrix green and styling */
    fps_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(fps_label),
                        "<span foreground='#00FF41' size='large'><b>⚡ FPS: --</b></span>");
    gtk_widget_set_margin_start(fps_label, 8);
    gtk_widget_set_margin_end(fps_label, 8);
    gtk_box_pack_end(GTK_BOX(toolbar), fps_label, FALSE, FALSE, 0);

    /* Paned window for editor and preview */
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(main_box), paned, TRUE, TRUE, 0);

    /* Left side: Editor with frame */
    GtkWidget *editor_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(editor_box, 4);
    gtk_widget_set_margin_end(editor_box, 2);
    gtk_widget_set_margin_top(editor_box, 4);
    gtk_widget_set_margin_bottom(editor_box, 4);

    /* Editor header */
    GtkWidget *editor_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *editor_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(editor_title),
                        "<span size='large'><b>📝 Shader Code</b></span>");
    gtk_widget_set_halign(editor_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(editor_header), editor_title, FALSE, FALSE, 0);

    /* Add help text */
    GtkWidget *help_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(help_label),
                        "<span size='small' alpha='60%'>Shadertoy-compatible GLSL</span>");
    gtk_box_pack_end(GTK_BOX(editor_header), help_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(editor_box), editor_header, FALSE, FALSE, 0);

    /* Source view with scrolled window */
    source_buffer = gtk_source_buffer_new(NULL);
    source_view = gtk_source_view_new_with_buffer(source_buffer);
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(source_view), TRUE);
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(source_view), editor_tab_width);
    gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(source_view), TRUE);
    gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(source_view), TRUE);
    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(source_view), TRUE);
    gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(source_view), TRUE);
    gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(source_view), 100);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(source_view), TRUE);

    /* Enable bracket matching */
    gtk_source_buffer_set_highlight_matching_brackets(source_buffer, TRUE);

    /* Set larger font */
    GtkCssProvider *font_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(font_provider,
        "textview { font-family: monospace; font-size: 11pt; }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(source_view),
        GTK_STYLE_PROVIDER(font_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(font_provider);

    /* Try to set GLSL language */
    GtkSourceLanguageManager *lang_mgr = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lang_mgr, "glsl");
    if (lang) {
        gtk_source_buffer_set_language(source_buffer, lang);
    }

    /* Set dark scheme */
    GtkSourceStyleSchemeManager *scheme_mgr = gtk_source_style_scheme_manager_get_default();
    GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(scheme_mgr, "oblivion");
    if (scheme) {
        gtk_source_buffer_set_style_scheme(source_buffer, scheme);
    }

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), source_view);
    gtk_box_pack_start(GTK_BOX(editor_box), scroll, TRUE, TRUE, 0);

    /* Error label */
    error_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(error_label), 0.0);
    gtk_widget_set_margin_start(error_label, 8);
    gtk_widget_set_margin_end(error_label, 8);
    gtk_widget_set_margin_top(error_label, 4);
    gtk_widget_set_margin_bottom(error_label, 4);
    gtk_box_pack_start(GTK_BOX(editor_box), error_label, FALSE, FALSE, 0);

    gtk_paned_add1(GTK_PANED(paned), editor_box);

    /* Right side: OpenGL preview with frame */
    GtkWidget *preview_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(preview_box, 2);
    gtk_widget_set_margin_end(preview_box, 4);
    gtk_widget_set_margin_top(preview_box, 4);
    gtk_widget_set_margin_bottom(preview_box, 4);

    /* Preview header */
    GtkWidget *preview_header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(preview_header),
                        "<span size='large'><b>👁️  Live Preview</b></span>");
    gtk_widget_set_halign(preview_header, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(preview_box), preview_header, FALSE, FALSE, 0);

    gl_area = gtk_gl_area_new();
    gtk_gl_area_set_required_version(GTK_GL_AREA(gl_area), 3, 2);
    gtk_gl_area_set_auto_render(GTK_GL_AREA(gl_area), TRUE);
    gtk_widget_set_size_request(gl_area, 500, 400);
    gtk_widget_add_events(gl_area, GDK_POINTER_MOTION_MASK);

    /* Add frame around GL area */
    GtkWidget *gl_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(gl_frame), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(gl_frame), gl_area);
    gtk_box_pack_start(GTK_BOX(preview_box), gl_frame, TRUE, TRUE, 0);

    gtk_paned_add2(GTK_PANED(paned), preview_box);
    gtk_paned_set_position(GTK_PANED(paned), 700);

    /* Status bar */
    GtkWidget *statusbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, STATUSBAR_SPACING);
    gtk_widget_set_margin_start(statusbar, STATUSBAR_MARGIN_H);
    gtk_widget_set_margin_end(statusbar, STATUSBAR_MARGIN_H);
    gtk_widget_set_margin_top(statusbar, STATUSBAR_MARGIN_V);
    gtk_widget_set_margin_bottom(statusbar, STATUSBAR_MARGIN_V);
    gtk_box_pack_start(GTK_BOX(main_box), statusbar, FALSE, FALSE, 0);

    status_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(status_label), "<span foreground='#00FF41'>✓ Ready</span>");
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
    gtk_box_pack_start(GTK_BOX(statusbar), status_label, TRUE, TRUE, 0);

    cursor_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(cursor_label),
                        "<span size='small'>📍 Line 1, Col 1</span>");
    gtk_box_pack_end(GTK_BOX(statusbar), cursor_label, FALSE, FALSE, 0);

    /* Connect signals */
    g_signal_connect(editor_window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    g_signal_connect(new_btn, "clicked", G_CALLBACK(on_reset_clicked), NULL);
    g_signal_connect(load_btn, "clicked", G_CALLBACK(on_load_clicked), NULL);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_clicked), NULL);
    g_signal_connect(install_btn, "clicked", G_CALLBACK(on_install_clicked), NULL);
    g_signal_connect(compile_btn, "clicked", G_CALLBACK(on_compile_clicked), NULL);
    g_signal_connect(pause_btn, "toggled", G_CALLBACK(on_pause_toggled), NULL);
    g_signal_connect(source_buffer, "changed", G_CALLBACK(on_buffer_changed), NULL);
    g_signal_connect(source_buffer, "notify::cursor-position", G_CALLBACK(on_cursor_moved), NULL);
    g_signal_connect(gl_area, "realize", G_CALLBACK(on_gl_realize), NULL);
    g_signal_connect(gl_area, "render", G_CALLBACK(on_gl_render), NULL);
    g_signal_connect(gl_area, "unrealize", G_CALLBACK(on_gl_unrealize), NULL);
    g_signal_connect(gl_area, "motion-notify-event", G_CALLBACK(on_preview_motion), NULL);

    /* Set default shader */
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(source_buffer), default_shader, -1);

    /* Show window */
    gtk_widget_show_all(editor_window);
}

/* Close shader editor */
void shader_editor_close(void) {
    if (editor_window) {
        gtk_widget_destroy(editor_window);
        editor_window = NULL;
    }
}

/* Check if editor is open */
bool shader_editor_is_open(void) {
    return editor_window != NULL;
}
