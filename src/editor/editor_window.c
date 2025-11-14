/* Main Window Component - Implementation
 * Orchestrates all editor components into the main window
 */

#include "editor_window.h"
#include "editor_text.h"
#include "editor_preview.h"
#include "editor_toolbar.h"
#include "editor_statusbar.h"
#include "editor_settings.h"
#include "editor_error_panel.h"
#include "file_operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Module state */
static struct {
    GtkWidget *window;
    GtkWidget *text_widget;
    GtkWidget *preview_widget;
    GtkWidget *toolbar_widget;
    GtkWidget *statusbar_widget;
    GtkWidget *error_panel_widget;
    GtkWidget *paned_widget;
    char *current_file;
    bool is_modified;
    bool is_open;
    guint compile_timeout_id;
    guint fps_update_id;
} window_state = {
    .window = NULL,
    .text_widget = NULL,
    .preview_widget = NULL,
    .toolbar_widget = NULL,
    .statusbar_widget = NULL,
    .error_panel_widget = NULL,
    .paned_widget = NULL,
    .current_file = NULL,
    .is_modified = false,
    .is_open = false,
    .compile_timeout_id = 0,
    .fps_update_id = 0
};

/* Forward declarations */
static void on_text_changed(const char *text, gpointer user_data);
static void on_cursor_moved(int line, int column, gpointer user_data);
static void on_preview_error(const char *error, gpointer user_data);
static void on_gl_realized(GtkGLArea *area, gpointer user_data);
static gboolean compile_shader_delayed(gpointer user_data);
static gboolean update_fps_timer(gpointer user_data);
static void on_settings_changed(EditorSettings *settings, gpointer user_data);
static void on_error_status_clicked(gpointer user_data);
static void on_paned_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data);

/* Editor settings */
static EditorSettings editor_settings;

/* Flag to track if paned position needs to be reset to 50/50 */
static gboolean paned_needs_reset = TRUE;


/* Toolbar callbacks */
static void on_new_clicked(gpointer user_data) {
    (void)user_data;

    if (window_state.is_modified) {
        if (!editor_window_prompt_save_if_modified()) {
            return;
        }
    }

    editor_window_load_default_shader();

    if (window_state.current_file) {
        g_free(window_state.current_file);
        window_state.current_file = NULL;
    }

    /* Mark as saved - this is a fresh start */
    window_state.is_modified = false;
    editor_text_mark_saved();
    editor_statusbar_set_modified(false);
    editor_window_update_title(NULL, false);
    editor_statusbar_set_message("New shader created");
}

static void on_load_clicked(gpointer user_data) {
    (void)user_data;

    if (window_state.is_modified) {
        if (!editor_window_prompt_save_if_modified()) {
            return;
        }
    }

    char *filename = file_operations_load_dialog(GTK_WINDOW(window_state.window));
    if (!filename) {
        return;
    }

    char *error = NULL;
    char *code = file_operations_load_file(filename, &error);

    if (code) {
        /* Block change callback temporarily to prevent marking as modified */
        editor_text_set_change_callback(NULL, NULL);
        editor_text_set_code(code);
        g_free(code);

        if (window_state.current_file) {
            g_free(window_state.current_file);
        }
        window_state.current_file = filename;

        /* Mark as saved - we just loaded from disk */
        window_state.is_modified = false;
        editor_text_mark_saved();
        editor_statusbar_set_modified(false);

        /* Reconnect change callback */
        editor_text_set_change_callback(on_text_changed, NULL);

        editor_window_update_title(file_operations_get_filename(filename), false);
        editor_statusbar_set_message("Shader loaded successfully");

        editor_window_compile_shader();
    } else {
        file_operations_error_dialog(GTK_WINDOW(window_state.window),
                                     "Load Failed", error);
        g_free(error);
        g_free(filename);
    }
}

static void on_save_clicked(gpointer user_data) {
    (void)user_data;

    char *filename = window_state.current_file;

    if (!filename) {
        filename = file_operations_save_dialog(GTK_WINDOW(window_state.window), NULL);
        if (!filename) {
            return;
        }
    }

    char *code = editor_text_get_code();
    char *error = NULL;

    if (file_operations_save_file(filename, code, &error)) {
        if (!window_state.current_file) {
            window_state.current_file = filename;
        }

        /* Mark as saved */
        window_state.is_modified = false;
        editor_text_mark_saved();
        editor_statusbar_set_modified(false);

        editor_window_update_title(file_operations_get_filename(filename), false);
        editor_statusbar_set_message("Shader saved successfully");
    } else {
        file_operations_error_dialog(GTK_WINDOW(window_state.window),
                                     "Save Failed", error);
        g_free(error);
        if (filename != window_state.current_file) {
            g_free(filename);
        }
    }

    g_free(code);
}

static void on_compile_clicked(gpointer user_data) {
    (void)user_data;
    editor_window_compile_shader();
}

static void on_pause_clicked(gpointer user_data) {
    (void)user_data;
    bool paused = editor_toolbar_is_paused();
    editor_preview_set_paused(paused);
    editor_statusbar_set_message(paused ? "Preview paused" : "Preview playing");
}

static void on_reset_clicked(gpointer user_data) {
    (void)user_data;
    editor_preview_reset_time();
    editor_statusbar_set_message("Animation time reset");
}

static void on_install_clicked(gpointer user_data) {
    (void)user_data;

    char *code = editor_text_get_code();
    const char *name = window_state.current_file ?
                       file_operations_get_filename(window_state.current_file) :
                       "custom_shader";

    char *error = NULL;
    if (file_operations_install_to_neowall(code, name, &error)) {
        file_operations_info_dialog(GTK_WINDOW(window_state.window),
                                    "Success",
                                    "Shader installed to NeoWall successfully!");
        editor_statusbar_set_message("Shader installed to NeoWall");
    } else {
        file_operations_error_dialog(GTK_WINDOW(window_state.window),
                                     "Installation Failed", error);
        g_free(error);
    }

    g_free(code);
}

static void on_settings_clicked(gpointer user_data) {
    (void)user_data;

    /* Show settings dialog */
    editor_settings_show_dialog(
        GTK_WINDOW(window_state.window),
        &editor_settings,
        on_settings_changed,
        NULL
    );
}

static void on_toggle_split_clicked(gpointer user_data) {
    (void)user_data;

    /* Toggle split orientation */
    if (editor_settings.split_orientation == SPLIT_HORIZONTAL) {
        editor_settings.split_orientation = SPLIT_VERTICAL;
    } else {
        editor_settings.split_orientation = SPLIT_HORIZONTAL;
    }

    /* Save and apply */
    editor_settings_save(&editor_settings);
    on_settings_changed(&editor_settings, NULL);

    /* Update toolbar button icon */
    editor_toolbar_set_split_horizontal(editor_settings.split_orientation == SPLIT_HORIZONTAL);
}

static void on_view_mode_changed(ViewMode mode, gpointer user_data) {
    (void)user_data;

    if (!window_state.text_widget || !window_state.preview_widget) return;

    switch (mode) {
        case VIEW_MODE_BOTH:
            gtk_widget_show(window_state.text_widget);
            gtk_widget_show(window_state.preview_widget);
            editor_statusbar_set_message("Showing both editor and preview");
            break;

        case VIEW_MODE_EDITOR_ONLY:
            gtk_widget_show(window_state.text_widget);
            gtk_widget_hide(window_state.preview_widget);
            editor_statusbar_set_message("Editor only mode");
            break;

        case VIEW_MODE_PREVIEW_ONLY:
            gtk_widget_hide(window_state.text_widget);
            gtk_widget_show(window_state.preview_widget);
            editor_statusbar_set_message("Preview only mode");
            break;
    }
}

static void on_settings_changed(EditorSettings *settings, gpointer user_data) {
    (void)user_data;

    if (!settings) return;

    /* Apply font size to editor */
    editor_text_set_font_size(settings->font_size);

    /* Apply tab width */
    editor_text_set_tab_width(settings->tab_width);

    /* Apply shader speed */
    editor_preview_set_speed((float)settings->shader_speed);

    /* Update compile button visibility based on auto-compile setting */
    bool compile_visible = !settings->auto_compile;
    editor_toolbar_set_compile_visible(compile_visible);

    /* Handle split orientation change */
    if (window_state.paned_widget) {
        GtkOrientation current_orientation = gtk_orientable_get_orientation(
            GTK_ORIENTABLE(window_state.paned_widget));
        GtkOrientation new_orientation = (settings->split_orientation == SPLIT_HORIZONTAL)
            ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

        if (current_orientation != new_orientation) {
            /* Remove children from old paned */
            GtkWidget *child1 = window_state.text_widget;
            GtkWidget *child2 = window_state.preview_widget;

            g_object_ref(child1);
            g_object_ref(child2);
            gtk_container_remove(GTK_CONTAINER(window_state.paned_widget), child1);
            gtk_container_remove(GTK_CONTAINER(window_state.paned_widget), child2);

            /* Change orientation */
            gtk_orientable_set_orientation(GTK_ORIENTABLE(window_state.paned_widget), new_orientation);

            /* Re-add children */
            gtk_paned_pack1(GTK_PANED(window_state.paned_widget), child1, TRUE, TRUE);
            gtk_paned_pack2(GTK_PANED(window_state.paned_widget), child2, TRUE, TRUE);

            g_object_unref(child1);
            g_object_unref(child2);

            /* Request 50/50 split on next size allocation */
            paned_needs_reset = TRUE;
            gtk_widget_queue_resize(window_state.paned_widget);
            gtk_widget_show_all(window_state.paned_widget);
        }
    }

    g_message("Settings changed: auto_compile=%s, compile_button=%s",
              settings->auto_compile ? "ON" : "OFF",
              compile_visible ? "VISIBLE" : "HIDDEN");

    /* Update status message */
    char msg[256];
    snprintf(msg, sizeof(msg), "Settings updated (Auto-compile: %s, Compile button: %s)",
             settings->auto_compile ? "ON" : "OFF",
             compile_visible ? "Visible" : "Hidden");
    editor_statusbar_set_message(msg);
}

static void on_error_status_clicked(gpointer user_data) {
    (void)user_data;

    /* Show the error panel when status bar error is clicked */
    if (!editor_error_panel_is_visible()) {
        const char *error = editor_preview_get_error();
        if (error && strlen(error) > 0) {
            editor_error_panel_show(error);
        }
    }
}

/* Callback to set paned position to 50/50 after size allocation */
static void on_paned_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data) {
    (void)user_data;

    if (!paned_needs_reset) {
        return;
    }

    /* Set position to 50/50 split based on orientation */
    GtkOrientation orientation = gtk_orientable_get_orientation(GTK_ORIENTABLE(widget));
    int position;

    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        position = allocation->width / 2;
    } else {
        position = allocation->height / 2;
    }

    gtk_paned_set_position(GTK_PANED(widget), position);
    paned_needs_reset = FALSE;
}

static void on_exit_clicked(gpointer user_data) {
    (void)user_data;
    editor_window_close();
}

/* Internal callbacks */
static void on_text_changed(const char *text, gpointer user_data) {
    (void)text;
    (void)user_data;

    /* Check the actual text buffer modified state */
    bool is_modified = editor_text_is_modified();
    window_state.is_modified = is_modified;
    editor_statusbar_set_modified(is_modified);
    editor_window_update_title(
        window_state.current_file ? file_operations_get_filename(window_state.current_file) : NULL,
        is_modified
    );

    /* Auto-compile with debounce (only if auto-compile is enabled) */
    if (editor_settings.auto_compile) {
        if (window_state.compile_timeout_id) {
            g_source_remove(window_state.compile_timeout_id);
        }
        window_state.compile_timeout_id = g_timeout_add(500, compile_shader_delayed, NULL);
    }
}

static void on_cursor_moved(int line, int column, gpointer user_data) {
    (void)user_data;
    editor_statusbar_set_cursor_position(line, column);
}

static void on_preview_error(const char *error, gpointer user_data) {
    (void)user_data;

    /* Show brief error in status bar */
    editor_statusbar_set_error("❌ Shader compilation failed");

    /* Don't show error panel automatically - user can click status bar to see details */
    (void)error;
}

static void on_gl_realized(GtkGLArea *area, gpointer user_data) {
    (void)area;
    (void)user_data;

    /* Now that GL context is ready, compile the default shader */
    editor_window_compile_shader();
}

static gboolean compile_shader_delayed(gpointer user_data) {
    (void)user_data;
    editor_window_compile_shader();
    window_state.compile_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

static gboolean update_fps_timer(gpointer user_data) {
    (void)user_data;
    double fps = editor_preview_get_fps();
    editor_statusbar_set_fps(fps);
    return G_SOURCE_CONTINUE;
}

static gboolean on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    (void)widget;
    (void)event;
    (void)user_data;

    if (window_state.is_modified) {
        return !editor_window_prompt_save_if_modified();
    }

    return FALSE;
}

static void on_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    editor_window_destroy();
}

/* Public API */

GtkWidget *editor_window_create(GtkApplication *app, const editor_window_config_t *config) {
    if (window_state.is_open) {
        gtk_window_present(GTK_WINDOW(window_state.window));
        return window_state.window;
    }

    /* Use defaults if no config provided */
    int width = config ? config->width : 1400;
    int height = config ? config->height : 850;
    const char *title = config ? config->title : "NeoWall Shader Editor";

    /* Create main window */
    if (app) {
        window_state.window = gtk_application_window_new(app);
    } else {
        window_state.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    }

    gtk_window_set_title(GTK_WINDOW(window_state.window), title);
    gtk_window_set_default_size(GTK_WINDOW(window_state.window), width, height);
    g_signal_connect(window_state.window, "delete-event", G_CALLBACK(on_delete_event), NULL);
    g_signal_connect(window_state.window, "destroy", G_CALLBACK(on_destroy), NULL);

    /* Create main container */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window_state.window), vbox);

    /* Create toolbar */
    editor_toolbar_callbacks_t toolbar_callbacks = {
        .on_new = on_new_clicked,
        .on_load = on_load_clicked,
        .on_save = on_save_clicked,
        .on_compile = on_compile_clicked,
        .on_pause = on_pause_clicked,
        .on_reset = on_reset_clicked,
        .on_install = on_install_clicked,
        .on_settings = on_settings_clicked,
        .on_exit = on_exit_clicked,
        .on_toggle_split = on_toggle_split_clicked,
        .on_view_mode_changed = on_view_mode_changed,
        .user_data = NULL
    };
    window_state.toolbar_widget = editor_toolbar_create(&toolbar_callbacks);
    gtk_box_pack_start(GTK_BOX(vbox), window_state.toolbar_widget, FALSE, FALSE, 0);

    /* Load editor settings BEFORE creating editor */
    editor_settings_load(&editor_settings);

    /* Create paned for editor and preview */
    GtkOrientation orientation = (editor_settings.split_orientation == SPLIT_HORIZONTAL)
        ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
    window_state.paned_widget = gtk_paned_new(orientation);
    gtk_box_pack_start(GTK_BOX(vbox), window_state.paned_widget, TRUE, TRUE, 0);

    /* Connect signal to set position after size allocation */
    g_signal_connect(window_state.paned_widget, "size-allocate",
                     G_CALLBACK(on_paned_size_allocate), NULL);

    /* Create text editor */
    window_state.text_widget = editor_text_create(NULL);
    /* Note: callbacks connected after initial content is loaded */
    gtk_paned_pack1(GTK_PANED(window_state.paned_widget), window_state.text_widget, TRUE, TRUE);

    /* Create preview */
    window_state.preview_widget = editor_preview_create();
    editor_preview_set_error_callback(on_preview_error, NULL);

    /* Connect to GL realize signal to compile shader when context is ready */
    g_signal_connect(window_state.preview_widget, "realize",
                     G_CALLBACK(on_gl_realized), NULL);

    gtk_paned_pack2(GTK_PANED(window_state.paned_widget), window_state.preview_widget, TRUE, TRUE);

    /* Create status bar */
    window_state.statusbar_widget = editor_statusbar_create();
    gtk_box_pack_start(GTK_BOX(vbox), window_state.statusbar_widget, FALSE, FALSE, 0);

    /* Create error panel (initially hidden) */
    window_state.error_panel_widget = editor_error_panel_create();
    gtk_box_pack_start(GTK_BOX(vbox), window_state.error_panel_widget, FALSE, FALSE, 0);

    /* Set error click callback on status bar */
    editor_statusbar_set_error_click_callback(on_error_status_clicked, NULL);

    /* Apply initial settings to editor */
    editor_text_set_font_size(editor_settings.font_size);
    editor_text_set_tab_width(editor_settings.tab_width);

    /* Apply shader speed to preview */
    editor_preview_set_speed((float)editor_settings.shader_speed);

    /* Set default shader text (compilation will happen when GL realizes) */
    /* Note: Don't connect callbacks yet to prevent marking as modified */
    editor_text_set_code(default_shader);

    /* Mark as NOT modified - this is the initial default state */
    window_state.is_modified = false;
    editor_text_mark_saved();
    editor_statusbar_set_modified(false);

    /* Now connect text change callbacks after initial content is loaded */
    editor_text_set_change_callback(on_text_changed, NULL);
    editor_text_set_cursor_callback(on_cursor_moved, NULL);

    /* Start FPS update timer */
    window_state.fps_update_id = g_timeout_add(100, update_fps_timer, NULL);

    window_state.is_open = true;

    gtk_widget_show_all(window_state.window);

    /* Set compile button visibility AFTER widgets are shown */
    editor_toolbar_set_compile_visible(!editor_settings.auto_compile);

    /* Initialize toolbar view button states */
    editor_toolbar_set_split_horizontal(editor_settings.split_orientation == SPLIT_HORIZONTAL);
    editor_toolbar_set_view_mode(VIEW_MODE_BOTH);

    /* Log initial settings state */
    g_message("Settings loaded: auto_compile=%s, compile_button=%s",
              editor_settings.auto_compile ? "ON" : "OFF",
              editor_settings.auto_compile ? "HIDDEN" : "VISIBLE");

    return window_state.window;
}

void editor_window_show(GtkApplication *app) {
    editor_window_create(app, NULL);
}

void editor_window_close(void) {
    if (!window_state.is_open) {
        return;
    }

    if (window_state.is_modified) {
        if (!editor_window_prompt_save_if_modified()) {
            return;
        }
    }

    if (window_state.window) {
        gtk_widget_destroy(window_state.window);
    }
}

bool editor_window_is_open(void) {
    return window_state.is_open;
}

GtkWidget *editor_window_get_widget(void) {
    return window_state.window;
}

void editor_window_set_title(const char *title) {
    if (window_state.window && title) {
        gtk_window_set_title(GTK_WINDOW(window_state.window), title);
    }
}

void editor_window_update_title(const char *filename, bool modified) {
    if (!window_state.window) {
        return;
    }

    char title[256];
    const char *name = filename ? filename : "Untitled";
    const char *mod = modified ? " *" : "";

    snprintf(title, sizeof(title), "%s%s - NeoWall Shader Editor", name, mod);
    gtk_window_set_title(GTK_WINDOW(window_state.window), title);
}

const char *editor_window_get_current_file(void) {
    return window_state.current_file;
}

void editor_window_set_current_file(const char *path) {
    if (window_state.current_file) {
        g_free(window_state.current_file);
    }
    window_state.current_file = path ? g_strdup(path) : NULL;
}

bool editor_window_is_modified(void) {
    /* Always check the text buffer for most accurate state */
    return editor_text_is_modified();
}

void editor_window_set_modified(bool modified) {
    window_state.is_modified = modified;
    if (modified) {
        /* Don't force mark as modified if it's not */
    } else {
        /* Mark as saved */
        editor_text_mark_saved();
    }
    editor_statusbar_set_modified(modified);
}

bool editor_window_prompt_save_if_modified(void) {
    if (!window_state.is_modified) {
        return true;
    }

    bool result = file_operations_confirm_dialog(
        GTK_WINDOW(window_state.window),
        "Unsaved Changes",
        "You have unsaved changes. Do you want to save before continuing?"
    );

    if (result) {
        on_save_clicked(NULL);
    }

    return true;
}

void editor_window_load_default_shader(void) {
    /* Block callbacks temporarily */
    editor_text_set_change_callback(NULL, NULL);
    editor_text_set_code(default_shader);

    /* Mark as saved - this is a clean slate */
    window_state.is_modified = false;
    editor_text_mark_saved();
    editor_statusbar_set_modified(false);

    /* Reconnect callback */
    editor_text_set_change_callback(on_text_changed, NULL);

    editor_window_compile_shader();
}

bool editor_window_compile_shader(void) {
    char *code = editor_text_get_code();
    bool success = editor_preview_compile_shader(code);

    if (success) {
        editor_statusbar_set_message("✓ Shader compiled successfully");
        /* Hide error panel on successful compilation */
        editor_error_panel_hide();
    } else {
        /* Show brief error in status bar - user can click to see details */
        editor_statusbar_set_error("❌ Compilation failed");

        /* Don't show error panel automatically - only on status bar click */
    }

    g_free(code);
    return success;
}

char *editor_window_get_shader_code(void) {
    return editor_text_get_code();
}

void editor_window_set_shader_code(const char *code) {
    editor_text_set_code(code);
}

void editor_window_toggle_pause(void) {
    bool paused = !editor_preview_is_paused();
    editor_preview_set_paused(paused);
    editor_toolbar_set_paused(paused);
}

void editor_window_reset_time(void) {
    editor_preview_reset_time();
}

void editor_window_destroy(void) {
    if (!window_state.is_open) {
        return;
    }

    /* Stop timers */
    if (window_state.compile_timeout_id) {
        g_source_remove(window_state.compile_timeout_id);
        window_state.compile_timeout_id = 0;
    }

    if (window_state.fps_update_id) {
        g_source_remove(window_state.fps_update_id);
        window_state.fps_update_id = 0;
    }

    /* Destroy components */
    editor_text_destroy();
    editor_preview_destroy();
    editor_toolbar_destroy();
    editor_statusbar_destroy();
    editor_error_panel_destroy();

    /* Free resources */
    if (window_state.current_file) {
        g_free(window_state.current_file);
        window_state.current_file = NULL;
    }

    window_state.window = NULL;
    window_state.text_widget = NULL;
    window_state.preview_widget = NULL;
    window_state.toolbar_widget = NULL;
    window_state.statusbar_widget = NULL;
    window_state.error_panel_widget = NULL;
    window_state.is_modified = false;
    window_state.is_open = false;
}
