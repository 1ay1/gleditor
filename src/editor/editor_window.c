/* Main Window Component - Implementation
 * Orchestrates all editor components into the main window
 */

#include "editor_window.h"
#include "editor_text.h"
#include "editor_preview.h"
#include "editor_toolbar.h"
#include "editor_statusbar.h"
#include "editor_error_panel.h"
#include "editor_settings.h"
#include "editor_help.h"
#include "editor_templates.h"
#include "editor_tabs.h"
#include "file_operations.h"
#include "keyboard_shortcuts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Default shader template */
static const char *default_shader =
    "// Cosmic Tunnel - NeoWall Shader Editor Demo\n"
    "// A mesmerizing raymarched tunnel with flowing energy\n"
    "\n"
    "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
    "    // Normalized coordinates centered at origin\n"
    "    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;\n"
    "    \n"
    "    // Create rotating tunnel effect\n"
    "    float t = iTime * 0.5;\n"
    "    float angle = atan(uv.y, uv.x);\n"
    "    float radius = length(uv);\n"
    "    \n"
    "    // Tunnel depth with perspective\n"
    "    float depth = 1.0 / (radius + 0.1);\n"
    "    \n"
    "    // Animated tunnel coordinates\n"
    "    vec2 tunnel = vec2(angle * 3.0, depth + t * 2.0);\n"
    "    \n"
    "    // Flowing energy patterns\n"
    "    float pattern = sin(tunnel.x * 4.0 + tunnel.y * 2.0) * 0.5 + 0.5;\n"
    "    pattern *= sin(tunnel.x * 2.0 - tunnel.y * 3.0 + t) * 0.5 + 0.5;\n"
    "    \n"
    "    // Circular rings\n"
    "    float rings = sin(depth * 10.0 - t * 4.0) * 0.5 + 0.5;\n"
    "    rings = pow(rings, 3.0);\n"
    "    \n"
    "    // Radial glow\n"
    "    float glow = 1.0 - smoothstep(0.0, 2.0, radius);\n"
    "    glow = pow(glow, 2.0);\n"
    "    \n"
    "    // Combine effects\n"
    "    float combined = pattern * rings + glow * 0.3;\n"
    "    \n"
    "    // Vibrant cosmic colors\n"
    "    vec3 color1 = vec3(0.5, 0.0, 1.0);  // Purple\n"
    "    vec3 color2 = vec3(0.0, 0.8, 1.0);  // Cyan\n"
    "    vec3 color3 = vec3(1.0, 0.2, 0.5);  // Pink\n"
    "    \n"
    "    // Color cycling based on position and time\n"
    "    vec3 col = mix(color1, color2, sin(tunnel.y * 0.5) * 0.5 + 0.5);\n"
    "    col = mix(col, color3, sin(tunnel.x * 0.3 + t) * 0.5 + 0.5);\n"
    "    \n"
    "    // Apply patterns and enhance brightness\n"
    "    col *= combined * 2.0;\n"
    "    \n"
    "    // Add sparkles\n"
    "    float sparkle = sin(tunnel.x * 20.0) * sin(tunnel.y * 15.0);\n"
    "    sparkle = pow(max(0.0, sparkle), 10.0);\n"
    "    col += vec3(sparkle) * 2.0;\n"
    "    \n"
    "    // Vignette effect\n"
    "    col *= 1.0 - radius * 0.3;\n"
    "    \n"
    "    // Output with gamma correction\n"
    "    fragColor = vec4(pow(col, vec3(0.8)), 1.0);\n"
    "}\n";

/* Module state */
static struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *editor_vbox;
    GtkWidget *text_widget;
    GtkWidget *preview_widget;
    GtkWidget *toolbar_widget;
    GtkWidget *statusbar_widget;
    GtkWidget *error_panel_widget;
    GtkWidget *paned_widget;
    bool is_open;
    guint compile_timeout_id;
    guint fps_update_id;
} window_state = {
    .window = NULL,
    .notebook = NULL,
    .editor_vbox = NULL,
    .text_widget = NULL,
    .preview_widget = NULL,
    .toolbar_widget = NULL,
    .statusbar_widget = NULL,
    .error_panel_widget = NULL,
    .paned_widget = NULL,
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
static void on_tab_changed(int tab_id, void *user_data);
static bool on_tab_close_request(int tab_id, void *user_data);

/* Editor settings */
static EditorSettings editor_settings;

/* Flag to track if paned position needs to be reset to 50/50 */
static gboolean paned_needs_reset = TRUE;

/* Callback implementations */

static void on_new_clicked(gpointer user_data) {
    (void)user_data;

    /* Show template selection dialog */
    char *selected_code = editor_templates_show_dialog(GTK_WINDOW(window_state.window));

    if (selected_code) {
        /* Create new tab with selected template */
        int tab_id = editor_tabs_new(NULL, selected_code);
        g_free(selected_code);

        if (tab_id >= 0) {
            editor_statusbar_set_message("New shader created from template");
        }
    }
}

static void on_load_clicked(gpointer user_data) {
    (void)user_data;

    char *filename = file_operations_load_dialog(GTK_WINDOW(window_state.window));
    if (!filename) {
        return;
    }

    char *error = NULL;
    char *code = file_operations_load_file(filename, &error);

    if (code) {
        /* Create new tab with loaded file */
        char *basename = g_path_get_basename(filename);
        int tab_id = editor_tabs_new(basename, code);
        g_free(basename);
        g_free(code);

        if (tab_id >= 0) {
            /* Set file path for the tab */
            editor_tabs_set_file_path(tab_id, filename);
            editor_tabs_set_modified(tab_id, false);
            editor_statusbar_set_message("Shader loaded successfully");
        }

        g_free(filename);
    } else {
        file_operations_error_dialog(GTK_WINDOW(window_state.window),
                                     "Load Failed", error);
        g_free(error);
        g_free(filename);
    }
}

static void on_save_clicked(gpointer user_data) {
    (void)user_data;

    /* Get current tab */
    int tab_id = editor_tabs_get_current();
    if (tab_id < 0) return;

    const TabInfo *info = editor_tabs_get_info(tab_id);
    if (!info) return;

    char *filename = info->file_path ? g_strdup(info->file_path) : NULL;

    if (!filename) {
        filename = file_operations_save_dialog(GTK_WINDOW(window_state.window), NULL);
        if (!filename) {
            return;
        }
    }

    char *code = editor_text_get_code();
    char *error = NULL;

    if (file_operations_save_file(filename, code, &error)) {
        /* Update tab with file path */
        editor_tabs_set_file_path(tab_id, filename);
        editor_tabs_set_modified(tab_id, false);

        /* Mark editor as saved */
        editor_text_mark_saved();
        editor_statusbar_set_modified(false);

        /* Update window title */
        editor_window_update_title(file_operations_get_filename(filename), false);

        editor_statusbar_set_message("Shader saved successfully");
    } else {
        file_operations_error_dialog(GTK_WINDOW(window_state.window),
                                     "Save Failed", error);
        g_free(error);
    }

    g_free(filename);
    g_free(code);
}

static void on_save_as_clicked(gpointer user_data) {
    (void)user_data;

    /* Get current tab */
    int tab_id = editor_tabs_get_current();
    if (tab_id < 0) return;

    const TabInfo *info = editor_tabs_get_info(tab_id);
    if (!info) return;

    /* Always prompt for new filename (even if file already has a path) */
    char *filename = file_operations_save_dialog(GTK_WINDOW(window_state.window),
                                                  info->file_path);
    if (!filename) {
        return;
    }

    char *code = editor_text_get_code();
    char *error = NULL;

    if (file_operations_save_file(filename, code, &error)) {
        /* Update tab with new file path */
        editor_tabs_set_file_path(tab_id, filename);
        editor_tabs_set_modified(tab_id, false);

        /* Mark editor as saved */
        editor_text_mark_saved();
        editor_statusbar_set_modified(false);

        /* Update window title */
        editor_window_update_title(file_operations_get_filename(filename), false);

        editor_statusbar_set_message("Shader saved successfully");
    } else {
        file_operations_error_dialog(GTK_WINDOW(window_state.window),
                                     "Save Failed", error);
        g_free(error);
    }

    g_free(filename);
    g_free(code);
}

static void on_compile_clicked(gpointer user_data) {
    (void)user_data;
    editor_window_compile_shader();
}

static void on_close_tab_clicked(gpointer user_data) {
    (void)user_data;

    /* Close current tab (with prompt if modified) */
    editor_tabs_close_current();
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

    int tab_id = editor_tabs_get_current();
    const TabInfo *info = (tab_id >= 0) ? editor_tabs_get_info(tab_id) : NULL;

    char *code = editor_text_get_code();
    const char *name = (info && info->file_path) ?
                       file_operations_get_filename(info->file_path) :
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

static void on_help_clicked(gpointer user_data) {
    (void)user_data;

    /* Show help dialog */
    editor_help_show_dialog(GTK_WINDOW(window_state.window));
}

static void on_toggle_error_panel(gpointer user_data) {
    (void)user_data;

    /* Toggle error panel visibility */
    if (window_state.error_panel_widget) {
        gboolean visible = gtk_widget_get_visible(window_state.error_panel_widget);
        if (visible) {
            editor_error_panel_hide();
        } else {
            /* Show with empty message - will show last error or "No errors" */
            editor_error_panel_show("");
        }
    }
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
            /* Resume rendering when preview is visible */
            editor_preview_set_paused(false);
            editor_statusbar_set_message("Preview visible - rendering active");
            break;

        case VIEW_MODE_EDITOR_ONLY:
            gtk_widget_show(window_state.text_widget);
            gtk_widget_hide(window_state.preview_widget);
            /* Pause rendering when preview is hidden to save resources */
            editor_preview_set_paused(true);
            editor_statusbar_set_message("Preview hidden - rendering paused");
            break;

        case VIEW_MODE_PREVIEW_ONLY:
            /* Hide only the editor, keep tabs visible */
            gtk_widget_hide(window_state.text_widget);
            gtk_widget_show(window_state.notebook);
            gtk_widget_show(window_state.preview_widget);
            /* Resume rendering when preview is visible */
            editor_preview_set_paused(false);
            editor_statusbar_set_message("Preview fullscreen with tabs");
            break;
    }
}

static void on_settings_changed(EditorSettings *settings, gpointer user_data) {
    (void)user_data;

    if (!settings) return;

    /* Apply all editor settings */
    editor_text_apply_all_settings(settings);

    /* Apply shader speed to preview */
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
    (void)user_data;

    /* Get current tab */
    int tab_id = editor_tabs_get_current();
    if (tab_id < 0) return;

    /* Update tab's code */
    editor_tabs_set_code(tab_id, text);

    /* Check the actual text buffer modified state */
    bool is_modified = editor_text_is_modified();
    editor_tabs_set_modified(tab_id, is_modified);
    editor_statusbar_set_modified(is_modified);

    /* Update window title */
    const TabInfo *info = editor_tabs_get_info(tab_id);
    if (info) {
        const char *filename = info->file_path ? file_operations_get_filename(info->file_path) : info->title;
        editor_window_update_title(filename, is_modified);
    }

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

/* Tab changed callback */
static void on_tab_changed(int tab_id, void *user_data) {
    (void)user_data;

    const TabInfo *info = editor_tabs_get_info(tab_id);
    if (!info) return;

    /* Update text editor with tab's code */
    editor_text_set_change_callback(NULL, NULL);
    editor_text_set_code(info->code);
    editor_text_set_change_callback(on_text_changed, NULL);

    /* Update modified flag */
    editor_statusbar_set_modified(info->is_modified);
    if (info->is_modified) {
        /* Mark as modified - no such function needed, editor_text handles it */
    } else {
        editor_text_mark_saved();
    }

    /* Update window title with current tab's file */
    const char *filename = info->file_path ? file_operations_get_filename(info->file_path) : info->title;
    editor_window_update_title(filename, info->is_modified);

    /* Auto-compile the shader for new tabs or recompile if already compiled before */
    if (info->code && strlen(info->code) > 0) {
        editor_window_compile_shader();
        editor_tabs_set_compiled(tab_id, true);
    }
}

/* Tab close request callback */
static bool on_tab_close_request(int tab_id, void *user_data) {
    (void)user_data;

    const TabInfo *info = editor_tabs_get_info(tab_id);
    if (!info) return true;

    /* Check if modified and ask to save */
    if (info->is_modified) {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(window_state.window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_NONE,
            "Save changes to \"%s\" before closing?",
            info->title
        );

        gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                              "Close _Without Saving", GTK_RESPONSE_NO,
                              "_Cancel", GTK_RESPONSE_CANCEL,
                              "_Save", GTK_RESPONSE_YES,
                              NULL);

        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);

        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_CANCEL) {
            return false; /* Cancel close */
        } else if (response == GTK_RESPONSE_YES) {
            /* Save before closing - switch to this tab first */
            editor_tabs_switch_to(tab_id);
            on_save_clicked(NULL);
        }
    }

    return true; /* Allow close */
}

static gboolean on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    (void)widget;
    (void)event;
    (void)user_data;

    /* Save tab session if enabled */
    if (editor_settings.remember_open_tabs) {
        editor_tabs_save_session();
    }

    /* Check if any tabs have unsaved changes */
    int tab_count = editor_tabs_get_count();
    for (int i = 0; i < tab_count; i++) {
        /* Tabs will be closed one by one, each prompting if needed */
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
    /* Create main vertical box */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window_state.window), vbox);

    /* Create notebook for tabs */
    window_state.notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(window_state.notebook), TRUE);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(window_state.notebook), TRUE);

    /* Initialize tab manager */
    editor_tabs_init(GTK_NOTEBOOK(window_state.notebook));
    editor_tabs_set_changed_callback(on_tab_changed, NULL);
    editor_tabs_set_close_callback(on_tab_close_request, NULL);

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
        .on_help = on_help_clicked,
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

    /* Create vertical box for text editor side (notebook + editor) */
    window_state.editor_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Add notebook to editor side */
    gtk_box_pack_start(GTK_BOX(window_state.editor_vbox), window_state.notebook, FALSE, FALSE, 0);

    /* Create text editor with loaded settings */
    window_state.text_widget = editor_text_create(&editor_settings);
    /* Note: callbacks connected after initial content is loaded */
    gtk_box_pack_start(GTK_BOX(window_state.editor_vbox), window_state.text_widget, TRUE, TRUE, 0);

    /* Add editor side to paned */
    gtk_paned_pack1(GTK_PANED(window_state.paned_widget), window_state.editor_vbox, TRUE, TRUE);

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
    editor_text_apply_all_settings(&editor_settings);

    /* Apply shader speed to preview */
    editor_preview_set_speed((float)editor_settings.shader_speed);

    /* Connect text change callbacks before creating tabs */
    editor_text_set_change_callback(on_text_changed, NULL);
    editor_text_set_cursor_callback(on_cursor_moved, NULL);

    /* Restore tabs from session or create initial tab */
    bool restored = false;
    if (editor_settings.remember_open_tabs) {
        restored = editor_tabs_restore_session();
    }

    if (!restored) {
        /* Create initial tab with default shader if no session restored */
        int initial_tab = editor_tabs_new("Untitled", default_shader);

        /* Manually trigger tab changed callback since GTK doesn't fire switch-page for first tab */
        if (initial_tab >= 0) {
            on_tab_changed(initial_tab, NULL);
        }
    }

    /* Initialize keyboard shortcuts */
    keyboard_shortcuts_callbacks_t shortcuts_callbacks = {
        .on_new = on_new_clicked,
        .on_open = on_load_clicked,
        .on_save = on_save_clicked,
        .on_save_as = on_save_as_clicked,
        .on_close = on_close_tab_clicked,
        .on_exit = on_exit_clicked,
        .on_compile = on_compile_clicked,
        .on_toggle_error_panel = on_toggle_error_panel,
        .on_toggle_split = on_toggle_split_clicked,
        .on_view_mode_changed = on_view_mode_changed,
        .on_settings = on_settings_clicked,
        .on_help = on_help_clicked,
        .user_data = NULL
    };
    keyboard_shortcuts_init(window_state.window, &shortcuts_callbacks);

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

    /* Save tab session if enabled */
    if (editor_settings.remember_open_tabs) {
        editor_tabs_save_session();
    }

    /* Close all tabs (each will prompt if modified) */
    while (editor_tabs_get_count() > 0) {
        if (!editor_tabs_close_current()) {
            return; /* User cancelled */
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
    int tab_count = editor_tabs_get_count();

    if (tab_count > 0) {
        const char *name = filename ? filename : "Untitled";
        const char *mod = modified ? " *" : "";
        snprintf(title, sizeof(title), "%s%s - NeoWall Shader Editor (%d tabs)",
                 name, mod, tab_count);
    } else {
        snprintf(title, sizeof(title), "NeoWall Shader Editor");
    }

    gtk_window_set_title(GTK_WINDOW(window_state.window), title);
}

const char *editor_window_get_current_file(void) {
    int tab_id = editor_tabs_get_current();
    if (tab_id < 0) return NULL;

    const TabInfo *info = editor_tabs_get_info(tab_id);
    return info ? info->file_path : NULL;
}

void editor_window_set_current_file(const char *path) {
    int tab_id = editor_tabs_get_current();
    if (tab_id < 0) return;

    editor_tabs_set_file_path(tab_id, path);
}

bool editor_window_is_modified(void) {
    /* Always check the text buffer for most accurate state */
    return editor_text_is_modified();
}

void editor_window_set_modified(bool modified) {
    int tab_id = editor_tabs_get_current();
    if (tab_id >= 0) {
        editor_tabs_set_modified(tab_id, modified);
    }

    if (!modified) {
        /* Mark as saved */
        editor_text_mark_saved();
    }
    editor_statusbar_set_modified(modified);
}

bool editor_window_prompt_save_if_modified(void) {
    /* Get current tab */
    int tab_id = editor_tabs_get_current();
    if (tab_id < 0) return true;

    const TabInfo *info = editor_tabs_get_info(tab_id);
    if (!info || !info->is_modified) {
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

    return result;
}

void editor_window_load_default_shader(void) {
    /* Create new tab with default shader */
    editor_tabs_new("Untitled", default_shader);
}

bool editor_window_compile_shader(void) {
    char *code = editor_text_get_code();

    /* Safety check - ensure we have valid code to compile */
    if (!code || strlen(code) == 0) {
        g_free(code);
        return false;
    }

    bool success = editor_preview_compile_shader(code);

    if (success) {
        editor_statusbar_set_message("✓ Shader compiled successfully");
        /* Hide error panel on successful compilation */
        editor_error_panel_hide();

        /* Mark current tab as compiled */
        int tab_id = editor_tabs_get_current();
        if (tab_id >= 0) {
            editor_tabs_set_compiled(tab_id, true);
        }
    } else {
        /* Show brief error in status bar - user can click to see details */
        editor_statusbar_set_error("❌ Compilation failed");
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
    /* Cleanup handled by tab manager */
    editor_tabs_cleanup();

    window_state.window = NULL;
    window_state.notebook = NULL;
    window_state.editor_vbox = NULL;
    window_state.text_widget = NULL;
    window_state.preview_widget = NULL;
    window_state.toolbar_widget = NULL;
    window_state.statusbar_widget = NULL;
    window_state.error_panel_widget = NULL;
    window_state.is_open = false;
}
