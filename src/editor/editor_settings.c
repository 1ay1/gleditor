/* gleditor - Settings Management Implementation
 * Handles editor settings persistence and dialog
 */

#include "editor_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SETTINGS_FILE ".config/gleditor/settings.conf"

/* Save settings to config file */
void editor_settings_save(const EditorSettings *settings) {
    if (!settings) return;

    const char *home = getenv("HOME");
    if (!home) return;

    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/%s", home, SETTINGS_FILE);

    /* Create directory if it doesn't exist */
    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s/.config/gleditor", home);
    mkdir(dir_path, 0755);

    FILE *f = fopen(config_path, "w");
    if (!f) return;

    fprintf(f, "# gleditor settings\n");
    fprintf(f, "font_size=%d\n", settings->font_size);
    fprintf(f, "tab_width=%d\n", settings->tab_width);
    fprintf(f, "auto_compile=%d\n", settings->auto_compile ? 1 : 0);
    fprintf(f, "preview_fps=%d\n", settings->preview_fps);

    fclose(f);
}

/* Load settings from config file */
void editor_settings_load(EditorSettings *settings) {
    if (!settings) return;

    /* Set defaults first */
    settings->font_size = 12;
    settings->tab_width = 4;
    settings->auto_compile = true;
    settings->preview_fps = 60;

    const char *home = getenv("HOME");
    if (!home) return;

    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/%s", home, SETTINGS_FILE);

    FILE *f = fopen(config_path, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n') continue;

        int value;
        if (sscanf(line, "font_size=%d", &value) == 1) {
            if (value >= 8 && value <= 24) {
                settings->font_size = value;
            }
        } else if (sscanf(line, "tab_width=%d", &value) == 1) {
            if (value >= 2 && value <= 8) {
                settings->tab_width = value;
            }
        } else if (sscanf(line, "auto_compile=%d", &value) == 1) {
            settings->auto_compile = (value != 0);
        } else if (sscanf(line, "preview_fps=%d", &value) == 1) {
            if (value >= 15 && value <= 120) {
                settings->preview_fps = value;
            }
        }
    }

    fclose(f);
}

/* Callback wrappers */
typedef struct {
    EditorSettings *settings;
    void (*on_change)(EditorSettings *, gpointer);
    gpointer user_data;
} SettingsCallbackData;

/* Font size changed */
static void on_font_size_changed(GtkSpinButton *spin, gpointer data) {
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->font_size = gtk_spin_button_get_value_as_int(spin);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) {
        cb_data->on_change(cb_data->settings, cb_data->user_data);
    }
}

/* Tab width changed */
static void on_tab_width_changed(GtkSpinButton *spin, gpointer data) {
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->tab_width = gtk_spin_button_get_value_as_int(spin);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) {
        cb_data->on_change(cb_data->settings, cb_data->user_data);
    }
}

/* Auto-compile toggled */
static void on_auto_compile_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->auto_compile = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) {
        cb_data->on_change(cb_data->settings, cb_data->user_data);
    }
}

/* Show settings dialog */
void editor_settings_show_dialog(GtkWindow *parent,
                                  EditorSettings *settings,
                                  void (*on_change)(EditorSettings *, gpointer),
                                  gpointer user_data) {
    if (!settings) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "⚙️ Editor Settings",
        parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 350);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_add(GTK_CONTAINER(content), grid);

    int row = 0;

    /* Header */
    GtkWidget *header_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header_label),
        "<b>Editor Preferences</b>\n<small>Changes are applied instantly and saved automatically</small>");
    gtk_label_set_xalign(GTK_LABEL(header_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), header_label, 0, row, 2, 1);
    row++;

    /* Separator */
    GtkWidget *sep0 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(grid), sep0, 0, row, 2, 1);
    row++;

    /* Prepare callback data */
    static SettingsCallbackData cb_data;
    cb_data.settings = settings;
    cb_data.on_change = on_change;
    cb_data.user_data = user_data;

    /* Font size */
    GtkWidget *font_label = gtk_label_new("📝 Font Size:");
    gtk_widget_set_halign(font_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), font_label, 0, row, 1, 1);

    GtkWidget *font_spin = gtk_spin_button_new_with_range(8, 24, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(font_spin), settings->font_size);
    g_signal_connect(font_spin, "value-changed", G_CALLBACK(on_font_size_changed), &cb_data);
    gtk_grid_attach(GTK_GRID(grid), font_spin, 1, row, 1, 1);
    row++;

    /* Tab width */
    GtkWidget *tab_label = gtk_label_new("↹ Tab Width:");
    gtk_widget_set_halign(tab_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), tab_label, 0, row, 1, 1);

    GtkWidget *tab_spin = gtk_spin_button_new_with_range(2, 8, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab_spin), settings->tab_width);
    g_signal_connect(tab_spin, "value-changed", G_CALLBACK(on_tab_width_changed), &cb_data);
    gtk_grid_attach(GTK_GRID(grid), tab_spin, 1, row, 1, 1);
    row++;

    /* Auto-compile */
    GtkWidget *auto_label = gtk_label_new("⚡ Auto-Compile:");
    gtk_widget_set_halign(auto_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), auto_label, 0, row, 1, 1);

    GtkWidget *auto_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(auto_switch), settings->auto_compile);
    g_signal_connect(auto_switch, "notify::active", G_CALLBACK(on_auto_compile_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(grid), auto_switch, 1, row, 1, 1);
    row++;

    /* Separator */
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(grid), sep, 0, row, 2, 1);
    row++;

    /* Keyboard shortcuts info */
    GtkWidget *shortcuts_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(shortcuts_label),
        "<b>⌨️ Keyboard Shortcuts:</b>\n"
        "<small>"
        "Ctrl+S - Save\n"
        "Ctrl+O - Load\n"
        "Ctrl+N - New\n"
        "Ctrl+R / F5 - Compile\n"
        "Ctrl+I - Install to NeoWall\n"
        "Ctrl+Q - Exit\n"
        "Ctrl++ / Ctrl+- - Zoom font\n"
        "F11 - Fullscreen"
        "</small>");
    gtk_label_set_xalign(GTK_LABEL(shortcuts_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), shortcuts_label, 0, row, 2, 1);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
