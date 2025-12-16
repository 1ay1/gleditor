/* Editor Toolbar Component - Implementation
 * Provides the main toolbar with file operations and controls
 */

#include "editor_toolbar.h"
#include "platform_compat.h"
#include <stdlib.h>
#include <string.h>

/* Module state */
static struct {
    GtkWidget *toolbar;
    GtkWidget *pause_button;
    GtkWidget *compile_button;
    GtkWidget *install_button;
    GtkWidget *split_button;
    GtkWidget *view_both_button;
    GtkWidget *view_editor_button;
    GtkWidget *view_preview_button;
    editor_toolbar_callbacks_t callbacks;
    bool is_paused;
    bool is_horizontal;
    ViewMode current_view_mode;
    bool initialized;
} toolbar_state = {
    .toolbar = NULL,
    .pause_button = NULL,
    .compile_button = NULL,
    .install_button = NULL,
    .split_button = NULL,
    .view_both_button = NULL,
    .view_editor_button = NULL,
    .view_preview_button = NULL,
    .callbacks = {0},
    .is_paused = false,
    .is_horizontal = true,
    .current_view_mode = VIEW_MODE_BOTH,
    .initialized = false
};

/* Internal callback wrappers */
static void on_new_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_new) {
        toolbar_state.callbacks.on_new(toolbar_state.callbacks.user_data);
    }
}

static void on_load_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_load) {
        toolbar_state.callbacks.on_load(toolbar_state.callbacks.user_data);
    }
}

static void on_save_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_save) {
        toolbar_state.callbacks.on_save(toolbar_state.callbacks.user_data);
    }
}

static void on_compile_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_compile) {
        toolbar_state.callbacks.on_compile(toolbar_state.callbacks.user_data);
    }
}

static void on_pause_toggled(GtkToggleButton *button, gpointer user_data) {
    (void)user_data;
    toolbar_state.is_paused = gtk_toggle_button_get_active(button);
    if (toolbar_state.callbacks.on_pause) {
        toolbar_state.callbacks.on_pause(toolbar_state.callbacks.user_data);
    }
}

static void on_reset_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_reset) {
        toolbar_state.callbacks.on_reset(toolbar_state.callbacks.user_data);
    }
}

static void on_install_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_install) {
        toolbar_state.callbacks.on_install(toolbar_state.callbacks.user_data);
    }
}

static void on_settings_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_settings) {
        toolbar_state.callbacks.on_settings(toolbar_state.callbacks.user_data);
    }
}

static void on_help_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_help) {
        toolbar_state.callbacks.on_help(toolbar_state.callbacks.user_data);
    }
}

static void on_exit_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_exit) {
        toolbar_state.callbacks.on_exit(toolbar_state.callbacks.user_data);
    }
}

static void on_toggle_split_clicked(GtkToggleButton *button, gpointer user_data) {
    (void)user_data;

    /* Update state based on button */
    toolbar_state.is_horizontal = !gtk_toggle_button_get_active(button);

    if (toolbar_state.callbacks.on_toggle_split) {
        toolbar_state.callbacks.on_toggle_split(toolbar_state.callbacks.user_data);
    }
}

static void on_view_mode_toggled(GtkToggleButton *button, gpointer user_data) {
    /* Only process if button is being activated (not deactivated) */
    if (!gtk_toggle_button_get_active(button)) {
        return;
    }

    ViewMode mode = GPOINTER_TO_INT(user_data);
    toolbar_state.current_view_mode = mode;

    if (toolbar_state.callbacks.on_view_mode_changed) {
        toolbar_state.callbacks.on_view_mode_changed(mode, toolbar_state.callbacks.user_data);
    }
}

/* Helper: Create button with icon and label */
static GtkWidget *create_button(const char *icon_name, const char *label) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    if (icon_name) {
        GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
        gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    }

    if (label) {
        GtkWidget *label_widget = gtk_label_new(label);
        gtk_box_pack_start(GTK_BOX(box), label_widget, FALSE, FALSE, 0);
    }

    gtk_container_add(GTK_CONTAINER(button), box);
    gtk_widget_set_margin_start(button, 4);
    gtk_widget_set_margin_end(button, 4);

    return button;
}

/* Helper: Create toggle button with icon and label */
static GtkWidget *create_toggle_button(const char *icon_name, const char *label) {
    GtkWidget *button = gtk_toggle_button_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    if (icon_name) {
        GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
        gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    }

    if (label) {
        GtkWidget *label_widget = gtk_label_new(label);
        gtk_box_pack_start(GTK_BOX(box), label_widget, FALSE, FALSE, 0);
    }

    gtk_container_add(GTK_CONTAINER(button), box);
    gtk_widget_set_margin_start(button, 4);
    gtk_widget_set_margin_end(button, 4);

    return button;
}

/* Helper: Create separator */
static GtkWidget *create_separator(void) {
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_start(sep, 8);
    gtk_widget_set_margin_end(sep, 8);
    return sep;
}

/* Public API */

GtkWidget *editor_toolbar_create(const editor_toolbar_callbacks_t *callbacks) {
    if (toolbar_state.initialized) {
        g_warning("editor_toolbar_create: Toolbar already initialized");
        return toolbar_state.toolbar;
    }

    /* Store callbacks */
    if (callbacks) {
        toolbar_state.callbacks = *callbacks;
    }

    /* Create toolbar container */
    toolbar_state.toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(toolbar_state.toolbar, 12);
    gtk_widget_set_margin_end(toolbar_state.toolbar, 12);
    gtk_widget_set_margin_top(toolbar_state.toolbar, 12);
    gtk_widget_set_margin_bottom(toolbar_state.toolbar, 12);

    /* File operations group */
    GtkWidget *new_btn = create_button("document-new", "New");
    gtk_widget_set_tooltip_text(new_btn, "Create a new shader from template (Ctrl+N)");
    g_signal_connect(new_btn, "clicked", G_CALLBACK(on_new_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), new_btn, FALSE, FALSE, 0);

    GtkWidget *load_btn = create_button("document-open", "Load");
    gtk_widget_set_tooltip_text(load_btn, "Open an existing shader file (Ctrl+O)");
    g_signal_connect(load_btn, "clicked", G_CALLBACK(on_load_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), load_btn, FALSE, FALSE, 0);

    GtkWidget *save_btn = create_button("document-save", "Save");
    gtk_widget_set_tooltip_text(save_btn, "Save current shader to file (Ctrl+S)");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), save_btn, FALSE, FALSE, 0);

    /* Separator */
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), create_separator(), FALSE, FALSE, 0);

    /* Shader control group */
    toolbar_state.compile_button = create_button("system-run", "Compile");
    gtk_widget_set_tooltip_text(toolbar_state.compile_button, "Manually compile the shader (Ctrl+R or F5)\nVisible only when auto-compile is off");
    g_signal_connect(toolbar_state.compile_button, "clicked", G_CALLBACK(on_compile_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.compile_button, FALSE, FALSE, 0);

    toolbar_state.pause_button = create_toggle_button("media-playback-pause", "Pause");
    gtk_widget_set_tooltip_text(toolbar_state.pause_button, "Pause/resume shader animation (Space)");
    g_signal_connect(toolbar_state.pause_button, "toggled", G_CALLBACK(on_pause_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.pause_button, FALSE, FALSE, 0);

    GtkWidget *reset_btn = create_button("view-refresh", "Reset");
    gtk_widget_set_tooltip_text(reset_btn, "Reset animation time to zero (Ctrl+0)");
    g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), reset_btn, FALSE, FALSE, 0);

    /* Separator */
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), create_separator(), FALSE, FALSE, 0);

    /* Installation group - NeoWall only works on Linux */
#ifdef PLATFORM_LINUX
    toolbar_state.install_button = create_button("go-jump", "Install to NeoWall");
    gtk_widget_set_tooltip_text(toolbar_state.install_button, "Install shader to NeoWall wallpaper system (Ctrl+I)\nSaves to ~/.config/neowall/shaders/");
    g_signal_connect(toolbar_state.install_button, "clicked", G_CALLBACK(on_install_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.install_button, FALSE, FALSE, 0);
#else
    toolbar_state.install_button = NULL;
#endif

    /* Spacer to push right-aligned buttons to the end */
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), spacer, TRUE, TRUE, 0);

    /* Right-aligned buttons */
    /* View controls group */
    toolbar_state.split_button = create_toggle_button("view-split-left-right", NULL);
    gtk_widget_set_tooltip_text(toolbar_state.split_button, "Toggle split orientation between horizontal (side-by-side) and vertical (top-bottom)");
    g_signal_connect(toolbar_state.split_button, "toggled", G_CALLBACK(on_toggle_split_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.split_button, FALSE, FALSE, 0);

    /* View mode toggle button group (radio-style) */
    GtkWidget *view_group_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(view_group_box), "linked");

    toolbar_state.view_both_button = gtk_radio_button_new_with_label(NULL, "Both");
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(toolbar_state.view_both_button), FALSE);
    gtk_widget_set_tooltip_text(toolbar_state.view_both_button, "Show both editor and preview panels (default)");
    g_signal_connect(toolbar_state.view_both_button, "toggled",
                     G_CALLBACK(on_view_mode_toggled), GINT_TO_POINTER(VIEW_MODE_BOTH));
    gtk_box_pack_start(GTK_BOX(view_group_box), toolbar_state.view_both_button, FALSE, FALSE, 0);

    toolbar_state.view_editor_button = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(toolbar_state.view_both_button), "Editor");
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(toolbar_state.view_editor_button), FALSE);
    gtk_widget_set_tooltip_text(toolbar_state.view_editor_button, "Show only the code editor\nHides preview for focused coding");
    g_signal_connect(toolbar_state.view_editor_button, "toggled",
                     G_CALLBACK(on_view_mode_toggled), GINT_TO_POINTER(VIEW_MODE_EDITOR_ONLY));
    gtk_box_pack_start(GTK_BOX(view_group_box), toolbar_state.view_editor_button, FALSE, FALSE, 0);

    toolbar_state.view_preview_button = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(toolbar_state.view_both_button), "Preview");
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(toolbar_state.view_preview_button), FALSE);
    gtk_widget_set_tooltip_text(toolbar_state.view_preview_button, "Show only the shader preview\nPerfect for presentations and fullscreen viewing");
    g_signal_connect(toolbar_state.view_preview_button, "toggled",
                     G_CALLBACK(on_view_mode_toggled), GINT_TO_POINTER(VIEW_MODE_PREVIEW_ONLY));
    gtk_box_pack_start(GTK_BOX(view_group_box), toolbar_state.view_preview_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), view_group_box, FALSE, FALSE, 0);

    GtkWidget *help_btn = create_button("help-browser", "Help");
    gtk_widget_set_tooltip_text(help_btn, "Show help and keyboard shortcuts (F1)");
    g_signal_connect(help_btn, "clicked", G_CALLBACK(on_help_clicked), NULL);

    GtkWidget *settings_btn = create_button("preferences-system", "Settings");
    gtk_widget_set_tooltip_text(settings_btn, "Open settings dialog (Ctrl+,)\nConfigure font, tabs, auto-compile, speed, and layout");
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_settings_clicked), NULL);

    /* Pack settings and help buttons together */
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), settings_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), help_btn, FALSE, FALSE, 0);

    GtkWidget *exit_btn = create_button("application-exit", "Exit");
    gtk_widget_set_tooltip_text(exit_btn, "Exit the application (Ctrl+Q)\nPrompts to save if there are unsaved changes");
    g_signal_connect(exit_btn, "clicked", G_CALLBACK(on_exit_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), exit_btn, FALSE, FALSE, 0);

    toolbar_state.initialized = true;
    toolbar_state.is_paused = false;

    return toolbar_state.toolbar;
}

void editor_toolbar_set_paused(bool paused) {
    if (!toolbar_state.pause_button) {
        return;
    }

    toolbar_state.is_paused = paused;

    /* Block signal to prevent recursion */
    g_signal_handlers_block_by_func(toolbar_state.pause_button,
                                    G_CALLBACK(on_pause_toggled), NULL);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbar_state.pause_button), paused);

    g_signal_handlers_unblock_by_func(toolbar_state.pause_button,
                                      G_CALLBACK(on_pause_toggled), NULL);
}

bool editor_toolbar_is_paused(void) {
    return toolbar_state.is_paused;
}

void editor_toolbar_set_compile_sensitive(bool sensitive) {
    if (toolbar_state.compile_button) {
        gtk_widget_set_sensitive(toolbar_state.compile_button, sensitive);
    }
}

void editor_toolbar_set_compile_visible(bool visible) {
    if (toolbar_state.compile_button) {
        gtk_widget_set_visible(toolbar_state.compile_button, visible);
    }
}

void editor_toolbar_set_install_sensitive(bool sensitive) {
    if (toolbar_state.install_button) {
        gtk_widget_set_sensitive(toolbar_state.install_button, sensitive);
    }
}

GtkWidget *editor_toolbar_get_pause_button(void) {
    return toolbar_state.pause_button;
}

void editor_toolbar_set_split_horizontal(bool is_horizontal) {
    if (!toolbar_state.split_button) {
        return;
    }

    toolbar_state.is_horizontal = is_horizontal;

    /* Update button icon based on orientation */
    GtkWidget *box = gtk_bin_get_child(GTK_BIN(toolbar_state.split_button));
    GList *children = gtk_container_get_children(GTK_CONTAINER(box));

    if (children && children->data) {
        GtkWidget *icon = GTK_WIDGET(children->data);
        const char *icon_name = is_horizontal ? "view-split-left-right" : "view-split-top-bottom";
        gtk_image_set_from_icon_name(GTK_IMAGE(icon), icon_name, GTK_ICON_SIZE_BUTTON);
    }

    g_list_free(children);

    /* Update toggle button state */
    g_signal_handlers_block_by_func(toolbar_state.split_button,
                                    G_CALLBACK(on_toggle_split_clicked), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbar_state.split_button), !is_horizontal);
    g_signal_handlers_unblock_by_func(toolbar_state.split_button,
                                      G_CALLBACK(on_toggle_split_clicked), NULL);
}

void editor_toolbar_set_view_mode(ViewMode mode) {
    toolbar_state.current_view_mode = mode;

    /* Block signals to prevent recursion */
    g_signal_handlers_block_by_func(toolbar_state.view_both_button,
                                    G_CALLBACK(on_view_mode_toggled), NULL);
    g_signal_handlers_block_by_func(toolbar_state.view_editor_button,
                                    G_CALLBACK(on_view_mode_toggled), NULL);
    g_signal_handlers_block_by_func(toolbar_state.view_preview_button,
                                    G_CALLBACK(on_view_mode_toggled), NULL);

    /* Activate the appropriate button */
    switch (mode) {
        case VIEW_MODE_BOTH:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbar_state.view_both_button), TRUE);
            break;
        case VIEW_MODE_EDITOR_ONLY:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbar_state.view_editor_button), TRUE);
            break;
        case VIEW_MODE_PREVIEW_ONLY:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbar_state.view_preview_button), TRUE);
            break;
    }

    /* Unblock signals */
    g_signal_handlers_unblock_by_func(toolbar_state.view_both_button,
                                      G_CALLBACK(on_view_mode_toggled), NULL);
    g_signal_handlers_unblock_by_func(toolbar_state.view_editor_button,
                                      G_CALLBACK(on_view_mode_toggled), NULL);
    g_signal_handlers_unblock_by_func(toolbar_state.view_preview_button,
                                      G_CALLBACK(on_view_mode_toggled), NULL);
}

ViewMode editor_toolbar_get_view_mode(void) {
    return toolbar_state.current_view_mode;
}

void editor_toolbar_destroy(void) {
    if (!toolbar_state.initialized) {
        return;
    }

    /* Cleanup handled by GTK */
    toolbar_state.toolbar = NULL;
    toolbar_state.pause_button = NULL;
    toolbar_state.compile_button = NULL;
    toolbar_state.install_button = NULL;
    toolbar_state.split_button = NULL;
    toolbar_state.view_both_button = NULL;
    toolbar_state.view_editor_button = NULL;
    toolbar_state.view_preview_button = NULL;
    memset(&toolbar_state.callbacks, 0, sizeof(toolbar_state.callbacks));
    toolbar_state.is_paused = false;
    toolbar_state.is_horizontal = true;
    toolbar_state.current_view_mode = VIEW_MODE_BOTH;
    toolbar_state.initialized = false;
}
