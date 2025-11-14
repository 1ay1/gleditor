/* Toolbar Component - Implementation
 * Handles the editor toolbar with action buttons
 */

#include "editor_toolbar.h"
#include <stdlib.h>
#include <string.h>

/* Module state */
static struct {
    GtkWidget *toolbar;
    GtkWidget *pause_button;
    GtkWidget *compile_button;
    GtkWidget *install_button;
    GtkWidget *split_button;
    GtkWidget *editor_button;
    GtkWidget *preview_button;
    editor_toolbar_callbacks_t callbacks;
    bool is_paused;
    bool is_horizontal;
    bool editor_visible;
    bool preview_visible;
    bool initialized;
} toolbar_state = {
    .toolbar = NULL,
    .pause_button = NULL,
    .compile_button = NULL,
    .install_button = NULL,
    .split_button = NULL,
    .editor_button = NULL,
    .preview_button = NULL,
    .callbacks = {0},
    .is_paused = false,
    .is_horizontal = true,
    .editor_visible = true,
    .preview_visible = true,
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

static void on_exit_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_exit) {
        toolbar_state.callbacks.on_exit(toolbar_state.callbacks.user_data);
    }
}

static void on_toggle_split_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_toggle_split) {
        toolbar_state.callbacks.on_toggle_split(toolbar_state.callbacks.user_data);
    }
}

static void on_toggle_editor_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_toggle_editor) {
        toolbar_state.callbacks.on_toggle_editor(toolbar_state.callbacks.user_data);
    }
}

static void on_toggle_preview_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (toolbar_state.callbacks.on_toggle_preview) {
        toolbar_state.callbacks.on_toggle_preview(toolbar_state.callbacks.user_data);
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
    g_signal_connect(new_btn, "clicked", G_CALLBACK(on_new_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), new_btn, FALSE, FALSE, 0);

    GtkWidget *load_btn = create_button("document-open", "Load");
    g_signal_connect(load_btn, "clicked", G_CALLBACK(on_load_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), load_btn, FALSE, FALSE, 0);

    GtkWidget *save_btn = create_button("document-save", "Save");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), save_btn, FALSE, FALSE, 0);

    /* Separator */
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), create_separator(), FALSE, FALSE, 0);

    /* Shader control group */
    toolbar_state.compile_button = create_button("system-run", "Compile");
    g_signal_connect(toolbar_state.compile_button, "clicked", G_CALLBACK(on_compile_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.compile_button, FALSE, FALSE, 0);

    toolbar_state.pause_button = create_toggle_button("media-playback-pause", "Pause");
    g_signal_connect(toolbar_state.pause_button, "toggled", G_CALLBACK(on_pause_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.pause_button, FALSE, FALSE, 0);

    GtkWidget *reset_btn = create_button("view-refresh", "Reset");
    g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), reset_btn, FALSE, FALSE, 0);

    /* Separator */
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), create_separator(), FALSE, FALSE, 0);

    /* View controls group */
    toolbar_state.split_button = create_button("view-split-left-right", "⇆");
    gtk_widget_set_tooltip_text(toolbar_state.split_button, "Toggle Split Orientation (Horizontal/Vertical)");
    g_signal_connect(toolbar_state.split_button, "clicked", G_CALLBACK(on_toggle_split_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.split_button, FALSE, FALSE, 0);

    toolbar_state.editor_button = create_button("text-x-generic", "📝");
    gtk_widget_set_tooltip_text(toolbar_state.editor_button, "Toggle Editor Visibility");
    g_signal_connect(toolbar_state.editor_button, "clicked", G_CALLBACK(on_toggle_editor_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.editor_button, FALSE, FALSE, 0);

    toolbar_state.preview_button = create_button("video-display", "🎬");
    gtk_widget_set_tooltip_text(toolbar_state.preview_button, "Toggle Preview Visibility");
    g_signal_connect(toolbar_state.preview_button, "clicked", G_CALLBACK(on_toggle_preview_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.preview_button, FALSE, FALSE, 0);

    /* Separator */
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), create_separator(), FALSE, FALSE, 0);

    /* Installation group */
    toolbar_state.install_button = create_button("go-jump", "Install to NeoWall");
    g_signal_connect(toolbar_state.install_button, "clicked", G_CALLBACK(on_install_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), toolbar_state.install_button, FALSE, FALSE, 0);

    /* Spacer to push right-aligned buttons to the end */
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), spacer, TRUE, TRUE, 0);

    /* Right-aligned buttons */
    GtkWidget *settings_btn = create_button("preferences-system", "Settings");
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_settings_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar_state.toolbar), settings_btn, FALSE, FALSE, 0);

    GtkWidget *exit_btn = create_button("application-exit", "Exit");
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

void editor_toolbar_set_split_orientation(bool is_horizontal) {
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
}

void editor_toolbar_set_editor_visible(bool visible) {
    if (!toolbar_state.editor_button) {
        return;
    }

    toolbar_state.editor_visible = visible;

    /* Update button appearance to show state */
    if (visible) {
        gtk_widget_set_opacity(toolbar_state.editor_button, 1.0);
    } else {
        gtk_widget_set_opacity(toolbar_state.editor_button, 0.4);
    }
}

void editor_toolbar_set_preview_visible(bool visible) {
    if (!toolbar_state.preview_button) {
        return;
    }

    toolbar_state.preview_visible = visible;

    /* Update button appearance to show state */
    if (visible) {
        gtk_widget_set_opacity(toolbar_state.preview_button, 1.0);
    } else {
        gtk_widget_set_opacity(toolbar_state.preview_button, 0.4);
    }
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
    toolbar_state.editor_button = NULL;
    toolbar_state.preview_button = NULL;
    memset(&toolbar_state.callbacks, 0, sizeof(toolbar_state.callbacks));
    toolbar_state.is_paused = false;
    toolbar_state.is_horizontal = true;
    toolbar_state.editor_visible = true;
    toolbar_state.preview_visible = true;
    toolbar_state.initialized = false;
}
