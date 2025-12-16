/* gleditor - Settings Management Implementation
 * Handles editor settings persistence and dialog
 */

#include "editor_settings.h"
#include "platform_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Save settings to config file */
void editor_settings_save(const EditorSettings *settings) {
    if (!settings) return;

    char config_dir[PATH_MAX];
    platform_get_config_dir(config_dir, sizeof(config_dir));

    /* Create directory if it doesn't exist */
    platform_mkdir_recursive(config_dir);

    char config_path[PATH_MAX];
    platform_path_join(config_path, sizeof(config_path), config_dir, "settings.conf");

    FILE *f = fopen(config_path, "w");
    if (!f) return;

    fprintf(f, "# gleditor settings\n");
    fprintf(f, "# Editor Appearance\n");
    fprintf(f, "font_family=%s\n", settings->font_family);
    fprintf(f, "font_size=%d\n", settings->font_size);
    fprintf(f, "theme=%s\n", settings->theme);
    fprintf(f, "show_line_numbers=%d\n", settings->show_line_numbers ? 1 : 0);
    fprintf(f, "highlight_current_line=%d\n", settings->highlight_current_line ? 1 : 0);
    fprintf(f, "show_right_margin=%d\n", settings->show_right_margin ? 1 : 0);
    fprintf(f, "right_margin_position=%d\n", settings->right_margin_position);
    fprintf(f, "show_whitespace=%d\n", settings->show_whitespace ? 1 : 0);
    fprintf(f, "word_wrap=%d\n", settings->word_wrap ? 1 : 0);
    fprintf(f, "cursor_style=%d\n", settings->cursor_style);
    fprintf(f, "show_indent_guides=%d\n", settings->show_indent_guides ? 1 : 0);
    fprintf(f, "background_pattern=%d\n", settings->background_pattern ? 1 : 0);
    fprintf(f, "scroll_past_end=%d\n", settings->scroll_past_end ? 1 : 0);
    fprintf(f, "mark_occurrences=%d\n", settings->mark_occurrences ? 1 : 0);
    fprintf(f, "# Editor Behavior\n");
    fprintf(f, "tab_width=%d\n", settings->tab_width);
    fprintf(f, "insert_spaces=%d\n", settings->insert_spaces ? 1 : 0);
    fprintf(f, "auto_indent=%d\n", settings->auto_indent ? 1 : 0);
    fprintf(f, "smart_home_end=%d\n", settings->smart_home_end ? 1 : 0);
    fprintf(f, "bracket_matching=%d\n", settings->bracket_matching ? 1 : 0);
    fprintf(f, "auto_completion=%d\n", settings->auto_completion ? 1 : 0);
    fprintf(f, "# Compilation\n");
    fprintf(f, "auto_compile=%d\n", settings->auto_compile ? 1 : 0);
    fprintf(f, "# Preview\n");
    fprintf(f, "preview_fps=%d\n", settings->preview_fps);
    fprintf(f, "# Session\n");
    fprintf(f, "remember_open_tabs=%d\n", settings->remember_open_tabs ? 1 : 0);
    fprintf(f, "shader_speed=%.2f\n", settings->shader_speed);
    fprintf(f, "# Layout\n");
    fprintf(f, "split_orientation=%d\n", settings->split_orientation);

    fclose(f);
}

/* Load settings from config file */
void editor_settings_load(EditorSettings *settings) {
    if (!settings) return;

    /* Set defaults first */
    strncpy(settings->font_family, "Monospace", sizeof(settings->font_family) - 1);
    settings->font_size = 11;
    strncpy(settings->theme, "oblivion", sizeof(settings->theme) - 1);
    settings->show_line_numbers = true;
    settings->highlight_current_line = true;
    settings->show_right_margin = true;
    settings->right_margin_position = 80;
    settings->show_whitespace = false;
    settings->word_wrap = false;
    settings->cursor_style = CURSOR_STYLE_BLOCK;
    settings->show_indent_guides = true;
    settings->background_pattern = true;
    settings->scroll_past_end = true;
    settings->mark_occurrences = true;
    settings->tab_width = 4;
    settings->insert_spaces = true;
    settings->auto_indent = true;
    settings->smart_home_end = true;
    settings->bracket_matching = true;
    settings->auto_completion = true;
    settings->auto_compile = true;
    settings->preview_fps = 60;
    settings->shader_speed = 1.0;
    settings->split_orientation = SPLIT_HORIZONTAL;
    settings->remember_open_tabs = true;

    char config_dir[PATH_MAX];
    platform_get_config_dir(config_dir, sizeof(config_dir));

    char config_path[PATH_MAX];
    platform_path_join(config_path, sizeof(config_path), config_dir, "settings.conf");

    FILE *f = fopen(config_path, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n') continue;

        int value;
        double dvalue;
        char strvalue[256];

        if (strncmp(line, "font_family=", 12) == 0) {
            /* Read everything after '=' until newline */
            char *value = line + 12;
            /* Trim trailing newline */
            size_t len = strlen(value);
            if (len > 0 && value[len-1] == '\n') {
                value[len-1] = '\0';
            }
            strncpy(settings->font_family, value, sizeof(settings->font_family) - 1);
            settings->font_family[sizeof(settings->font_family) - 1] = '\0';
        } else if (sscanf(line, "font_size=%d", &value) == 1) {
            if (value >= 8 && value <= 24) {
                settings->font_size = value;
            }
        } else if (sscanf(line, "theme=%31s", strvalue) == 1) {
            strncpy(settings->theme, strvalue, sizeof(settings->theme) - 1);
            settings->theme[sizeof(settings->theme) - 1] = '\0';
        } else if (sscanf(line, "show_line_numbers=%d", &value) == 1) {
            settings->show_line_numbers = (value != 0);
        } else if (sscanf(line, "highlight_current_line=%d", &value) == 1) {
            settings->highlight_current_line = (value != 0);
        } else if (sscanf(line, "show_right_margin=%d", &value) == 1) {
            settings->show_right_margin = (value != 0);
        } else if (sscanf(line, "right_margin_position=%d", &value) == 1) {
            if (value >= 40 && value <= 120) {
                settings->right_margin_position = value;
            }
        } else if (sscanf(line, "show_whitespace=%d", &value) == 1) {
            settings->show_whitespace = (value != 0);
        } else if (sscanf(line, "word_wrap=%d", &value) == 1) {
            settings->word_wrap = (value != 0);
        } else if (sscanf(line, "cursor_style=%d", &value) == 1) {
            if (value >= 0 && value <= 1) {
                settings->cursor_style = (CursorStyle)value;
            }
        } else if (sscanf(line, "show_indent_guides=%d", &value) == 1) {
            settings->show_indent_guides = (value != 0);
        } else if (sscanf(line, "background_pattern=%d", &value) == 1) {
            settings->background_pattern = (value != 0);
        } else if (sscanf(line, "scroll_past_end=%d", &value) == 1) {
            settings->scroll_past_end = (value != 0);
        } else if (sscanf(line, "mark_occurrences=%d", &value) == 1) {
            settings->mark_occurrences = (value != 0);
        } else if (sscanf(line, "tab_width=%d", &value) == 1) {
            if (value >= 2 && value <= 8) {
                settings->tab_width = value;
            }
        } else if (sscanf(line, "insert_spaces=%d", &value) == 1) {
            settings->insert_spaces = (value != 0);
        } else if (sscanf(line, "auto_indent=%d", &value) == 1) {
            settings->auto_indent = (value != 0);
        } else if (sscanf(line, "smart_home_end=%d", &value) == 1) {
            settings->smart_home_end = (value != 0);
        } else if (sscanf(line, "bracket_matching=%d", &value) == 1) {
            settings->bracket_matching = (value != 0);
        } else if (sscanf(line, "auto_completion=%d", &value) == 1) {
            settings->auto_completion = (value != 0);
        } else if (sscanf(line, "auto_compile=%d", &value) == 1) {
            settings->auto_compile = (value != 0);
        } else if (sscanf(line, "remember_open_tabs=%d", &value) == 1) {
            settings->remember_open_tabs = (value != 0);
        } else if (sscanf(line, "preview_fps=%d", &value) == 1) {
            if (value >= 15 && value <= 120) {
                settings->preview_fps = value;
            }
        } else if (sscanf(line, "shader_speed=%lf", &dvalue) == 1) {
            if (dvalue >= 0.1 && dvalue <= 5.0) {
                settings->shader_speed = dvalue;
            }
        } else if (sscanf(line, "split_orientation=%d", &value) == 1) {
            if (value == 0 || value == 1) {
                settings->split_orientation = (SplitOrientation)value;
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
static void on_font_changed(GtkFontButton *font_button, gpointer data) {
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    const char *font_name = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(font_button));

    if (font_name) {
        /* Parse font name to get family and size */
        PangoFontDescription *font_desc = pango_font_description_from_string(font_name);
        const char *family = pango_font_description_get_family(font_desc);
        int size = pango_font_description_get_size(font_desc) / PANGO_SCALE;

        if (family) {
            strncpy(cb_data->settings->font_family, family, sizeof(cb_data->settings->font_family) - 1);
            cb_data->settings->font_family[sizeof(cb_data->settings->font_family) - 1] = '\0';
        }
        if (size > 0) {
            cb_data->settings->font_size = size;
        }

        pango_font_description_free(font_desc);

        editor_settings_save(cb_data->settings);
        if (cb_data->on_change) {
            cb_data->on_change(cb_data->settings, cb_data->user_data);
        }
    }
}

static void on_font_size_changed(GtkSpinButton *spin, gpointer data) __attribute__((unused));
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

static void on_remember_tabs_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->remember_open_tabs = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) {
        cb_data->on_change(cb_data->settings, cb_data->user_data);
    }
}

/* Shader speed changed */
static void on_shader_speed_changed(GtkSpinButton *spin, gpointer data) {
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->shader_speed = gtk_spin_button_get_value(spin);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) {
        cb_data->on_change(cb_data->settings, cb_data->user_data);
    }
}



/* Callback stubs for new settings */
static void on_theme_changed(GtkComboBox *combo, gpointer data) {
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    int active = gtk_combo_box_get_active(combo);
    const char *themes[] = {"oblivion", "classic", "cobalt", "kate", "solarized-dark", "solarized-light"};
    if (active >= 0 && active < 6) {
        strncpy(cb_data->settings->theme, themes[active], sizeof(cb_data->settings->theme) - 1);
        editor_settings_save(cb_data->settings);
        if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
    }
}

static void on_show_line_numbers_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->show_line_numbers = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_highlight_line_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->highlight_current_line = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_show_right_margin_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->show_right_margin = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_bracket_matching_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->bracket_matching = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_show_whitespace_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->show_whitespace = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_word_wrap_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->word_wrap = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_cursor_style_changed(GtkComboBox *combo, gpointer data) {
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->cursor_style = (CursorStyle)gtk_combo_box_get_active(combo);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_reset_speed_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkSpinButton *spin = GTK_SPIN_BUTTON(data);
    gtk_spin_button_set_value(spin, 1.0);
}

/* Removed - show_indent_guides was redundant with background_pattern */

static void on_background_pattern_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->background_pattern = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_scroll_past_end_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->scroll_past_end = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_mark_occurrences_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->mark_occurrences = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_auto_completion_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->auto_completion = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) cb_data->on_change(cb_data->settings, cb_data->user_data);
}

static void on_insert_spaces_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->insert_spaces = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) {
        cb_data->on_change(cb_data->settings, cb_data->user_data);
    }
}

static void on_auto_indent_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->auto_indent = gtk_switch_get_active(sw);
    editor_settings_save(cb_data->settings);
    if (cb_data->on_change) {
        cb_data->on_change(cb_data->settings, cb_data->user_data);
    }
}

static void on_smart_home_end_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    SettingsCallbackData *cb_data = (SettingsCallbackData *)data;
    cb_data->settings->smart_home_end = gtk_switch_get_active(sw);
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

    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 650);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    /* Prepare callback data */
    static SettingsCallbackData cb_data;
    cb_data.settings = settings;
    cb_data.on_change = on_change;
    cb_data.user_data = user_data;

    /* Create notebook for tabbed interface */
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(content), notebook);

    /* ===== APPEARANCE TAB ===== */
    GtkWidget *appearance_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(appearance_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(appearance_scroll), 400);

    GtkWidget *appearance_viewport = gtk_viewport_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(appearance_scroll), appearance_viewport);

    GtkWidget *appearance_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(appearance_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(appearance_grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(appearance_grid), 12);
    gtk_widget_set_hexpand(appearance_grid, TRUE);
    gtk_widget_set_vexpand(appearance_grid, TRUE);
    gtk_container_add(GTK_CONTAINER(appearance_viewport), appearance_grid);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), appearance_scroll, gtk_label_new("🎨 Appearance"));

    int row = 0;

    /* Font selector */
    GtkWidget *font_selector_label = gtk_label_new("Font:");
    gtk_widget_set_halign(font_selector_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), font_selector_label, 0, row, 1, 1);

    char current_font[128];
    snprintf(current_font, sizeof(current_font), "%s %d", settings->font_family, settings->font_size);
    GtkWidget *font_button = gtk_font_button_new_with_font(current_font);
    gtk_font_button_set_use_font(GTK_FONT_BUTTON(font_button), FALSE);
    gtk_font_button_set_use_size(GTK_FONT_BUTTON(font_button), TRUE);
    gtk_font_chooser_set_preview_text(GTK_FONT_CHOOSER(font_button), "vec3 color = vec3(1.0, 0.5, 0.0);");
    gtk_widget_set_tooltip_text(font_button, "Select editor font family and size");
    g_signal_connect(font_button, "font-set", G_CALLBACK(on_font_changed), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), font_button, 1, row, 1, 1);
    row++;

    /* Theme */
    GtkWidget *theme_label = gtk_label_new("Color Theme:");
    gtk_widget_set_halign(theme_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), theme_label, 0, row, 1, 1);

    GtkWidget *theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Oblivion (Dark)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Classic (Light)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Cobalt (Blue)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Kate (Dark)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Solarized Dark");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Solarized Light");

    if (strcmp(settings->theme, "classic") == 0) gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 1);
    else if (strcmp(settings->theme, "cobalt") == 0) gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 2);
    else if (strcmp(settings->theme, "kate") == 0) gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 3);
    else if (strcmp(settings->theme, "solarized-dark") == 0) gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 4);
    else if (strcmp(settings->theme, "solarized-light") == 0) gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 5);
    else gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 0);

    gtk_widget_set_tooltip_text(theme_combo, "Syntax highlighting color scheme");
    g_signal_connect(theme_combo, "changed", G_CALLBACK(on_theme_changed), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), theme_combo, 1, row, 1, 1);
    row++;

    /* Show line numbers */
    GtkWidget *line_num_label = gtk_label_new("Show Line Numbers:");
    gtk_widget_set_halign(line_num_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), line_num_label, 0, row, 1, 1);

    GtkWidget *line_num_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(line_num_switch), settings->show_line_numbers);
    gtk_widget_set_tooltip_text(line_num_switch, "Display line numbers in left margin");
    g_signal_connect(line_num_switch, "notify::active", G_CALLBACK(on_show_line_numbers_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), line_num_switch, 1, row, 1, 1);
    row++;

    /* Highlight current line */
    GtkWidget *highlight_label = gtk_label_new("Highlight Current Line:");
    gtk_widget_set_halign(highlight_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), highlight_label, 0, row, 1, 1);

    GtkWidget *highlight_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(highlight_switch), settings->highlight_current_line);
    gtk_widget_set_tooltip_text(highlight_switch, "Highlight the line where cursor is");
    g_signal_connect(highlight_switch, "notify::active", G_CALLBACK(on_highlight_line_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), highlight_switch, 1, row, 1, 1);
    row++;

    /* Show right margin */
    GtkWidget *margin_label = gtk_label_new("Show Right Margin:");
    gtk_widget_set_halign(margin_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), margin_label, 0, row, 1, 1);

    GtkWidget *margin_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(margin_switch), settings->show_right_margin);
    gtk_widget_set_tooltip_text(margin_switch, "Show vertical line at 80 characters");
    g_signal_connect(margin_switch, "notify::active", G_CALLBACK(on_show_right_margin_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), margin_switch, 1, row, 1, 1);
    row++;

    /* Show whitespace */
    GtkWidget *whitespace_label = gtk_label_new("Show Whitespace:");
    gtk_widget_set_halign(whitespace_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), whitespace_label, 0, row, 1, 1);

    GtkWidget *whitespace_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(whitespace_switch), settings->show_whitespace);
    gtk_widget_set_tooltip_text(whitespace_switch, "Display spaces and tabs visually");
    g_signal_connect(whitespace_switch, "notify::active", G_CALLBACK(on_show_whitespace_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), whitespace_switch, 1, row, 1, 1);
    row++;

    /* Word wrap */
    GtkWidget *wrap_label = gtk_label_new("Word Wrap:");
    gtk_widget_set_halign(wrap_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), wrap_label, 0, row, 1, 1);

    GtkWidget *wrap_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(wrap_switch), settings->word_wrap);
    gtk_widget_set_tooltip_text(wrap_switch, "Wrap long lines instead of horizontal scrolling");
    g_signal_connect(wrap_switch, "notify::active", G_CALLBACK(on_word_wrap_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), wrap_switch, 1, row, 1, 1);
    row++;

    /* Bracket matching */
    GtkWidget *bracket_label = gtk_label_new("Bracket Matching:");
    gtk_widget_set_halign(bracket_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), bracket_label, 0, row, 1, 1);

    GtkWidget *bracket_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(bracket_switch), settings->bracket_matching);
    gtk_widget_set_tooltip_text(bracket_switch, "Highlight matching brackets");
    g_signal_connect(bracket_switch, "notify::active", G_CALLBACK(on_bracket_matching_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), bracket_switch, 1, row, 1, 1);
    row++;

    /* Cursor style */
    GtkWidget *cursor_label = gtk_label_new("Cursor Style:");
    gtk_widget_set_halign(cursor_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), cursor_label, 0, row, 1, 1);

    GtkWidget *cursor_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cursor_combo), "Block (Overwrite Mode)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cursor_combo), "I-Beam (Insert Mode)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cursor_combo), settings->cursor_style);
    gtk_widget_set_tooltip_text(cursor_combo, "Cursor appearance style");
    g_signal_connect(cursor_combo, "changed", G_CALLBACK(on_cursor_style_changed), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), cursor_combo, 1, row, 1, 1);
    row++;

    /* Background pattern */
    GtkWidget *bg_pattern_label = gtk_label_new("Background Pattern:");
    gtk_widget_set_halign(bg_pattern_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), bg_pattern_label, 0, row, 1, 1);

    GtkWidget *bg_pattern_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(bg_pattern_switch), settings->background_pattern);
    gtk_widget_set_tooltip_text(bg_pattern_switch, "Show subtle grid pattern and indent guides in editor background");
    g_signal_connect(bg_pattern_switch, "notify::active", G_CALLBACK(on_background_pattern_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), bg_pattern_switch, 1, row, 1, 1);
    row++;

    /* Scroll past end */
    GtkWidget *scroll_past_label = gtk_label_new("Scroll Past End:");
    gtk_widget_set_halign(scroll_past_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), scroll_past_label, 0, row, 1, 1);

    GtkWidget *scroll_past_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(scroll_past_switch), settings->scroll_past_end);
    gtk_widget_set_tooltip_text(scroll_past_switch, "Allow scrolling beyond the last line");
    g_signal_connect(scroll_past_switch, "notify::active", G_CALLBACK(on_scroll_past_end_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), scroll_past_switch, 1, row, 1, 1);
    row++;

    /* Mark occurrences */
    GtkWidget *mark_occur_label = gtk_label_new("Mark Occurrences:");
    gtk_widget_set_halign(mark_occur_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(appearance_grid), mark_occur_label, 0, row, 1, 1);

    GtkWidget *mark_occur_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(mark_occur_switch), settings->mark_occurrences);
    gtk_widget_set_tooltip_text(mark_occur_switch, "Highlight all occurrences of selected text");
    g_signal_connect(mark_occur_switch, "notify::active", G_CALLBACK(on_mark_occurrences_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(appearance_grid), mark_occur_switch, 1, row, 1, 1);
    row++;

    /* ===== BEHAVIOR TAB ===== */
    GtkWidget *behavior_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(behavior_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(behavior_scroll), 400);

    GtkWidget *behavior_viewport = gtk_viewport_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(behavior_scroll), behavior_viewport);

    GtkWidget *behavior_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(behavior_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(behavior_grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(behavior_grid), 12);
    gtk_widget_set_hexpand(behavior_grid, TRUE);
    gtk_widget_set_vexpand(behavior_grid, TRUE);
    gtk_container_add(GTK_CONTAINER(behavior_viewport), behavior_grid);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), behavior_scroll, gtk_label_new("⚙️ Behavior"));

    row = 0;

    /* Tab width */
    GtkWidget *tab_label = gtk_label_new("Tab Width:");
    gtk_widget_set_halign(tab_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(behavior_grid), tab_label, 0, row, 1, 1);

    GtkWidget *tab_spin = gtk_spin_button_new_with_range(2, 8, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab_spin), settings->tab_width);
    gtk_widget_set_tooltip_text(tab_spin, "Number of spaces per tab (2-8)");
    g_signal_connect(tab_spin, "value-changed", G_CALLBACK(on_tab_width_changed), &cb_data);
    gtk_grid_attach(GTK_GRID(behavior_grid), tab_spin, 1, row, 1, 1);
    row++;

    /* Insert Spaces Instead of Tabs */
    GtkWidget *insert_spaces_label = gtk_label_new("Insert Spaces:");
    gtk_widget_set_halign(insert_spaces_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(behavior_grid), insert_spaces_label, 0, row, 1, 1);

    GtkWidget *insert_spaces_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(insert_spaces_switch), settings->insert_spaces);
    gtk_widget_set_tooltip_text(insert_spaces_switch, "Insert spaces instead of tabs when pressing Tab key");
    g_signal_connect(insert_spaces_switch, "notify::active", G_CALLBACK(on_insert_spaces_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(behavior_grid), insert_spaces_switch, 1, row, 1, 1);
    row++;

    /* Auto Indent */
    GtkWidget *auto_indent_label = gtk_label_new("Auto Indent:");
    gtk_widget_set_halign(auto_indent_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(behavior_grid), auto_indent_label, 0, row, 1, 1);

    GtkWidget *auto_indent_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(auto_indent_switch), settings->auto_indent);
    gtk_widget_set_tooltip_text(auto_indent_switch, "Automatically indent new lines to match previous line");
    g_signal_connect(auto_indent_switch, "notify::active", G_CALLBACK(on_auto_indent_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(behavior_grid), auto_indent_switch, 1, row, 1, 1);
    row++;

    /* Smart Home/End */
    GtkWidget *smart_home_label = gtk_label_new("Smart Home/End:");
    gtk_widget_set_halign(smart_home_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(behavior_grid), smart_home_label, 0, row, 1, 1);

    GtkWidget *smart_home_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(smart_home_switch), settings->smart_home_end);
    gtk_widget_set_tooltip_text(smart_home_switch, "Home key moves to first non-whitespace character before line start");
    g_signal_connect(smart_home_switch, "notify::active", G_CALLBACK(on_smart_home_end_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(behavior_grid), smart_home_switch, 1, row, 1, 1);
    row++;

    /* Auto-compile */
    GtkWidget *auto_label = gtk_label_new("Auto-Compile:");
    gtk_widget_set_halign(auto_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(behavior_grid), auto_label, 0, row, 1, 1);

    GtkWidget *auto_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(auto_switch), settings->auto_compile);
    gtk_widget_set_tooltip_text(auto_switch, "Compile shader automatically as you type (500ms delay)");
    g_signal_connect(auto_switch, "notify::active", G_CALLBACK(on_auto_compile_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(behavior_grid), auto_switch, 1, row, 1, 1);
    row++;

    /* Auto-completion */
    GtkWidget *completion_label = gtk_label_new("Auto-Completion:");
    gtk_widget_set_halign(completion_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(behavior_grid), completion_label, 0, row, 1, 1);

    GtkWidget *completion_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(completion_switch), settings->auto_completion);
    gtk_widget_set_tooltip_text(completion_switch, "Enable code completion for GLSL keywords and functions");
    g_signal_connect(completion_switch, "notify::active", G_CALLBACK(on_auto_completion_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(behavior_grid), completion_switch, 1, row, 1, 1);
    row++;

    /* Remember open tabs */
    GtkWidget *remember_tabs_label = gtk_label_new("Remember Open Tabs:");
    gtk_widget_set_halign(remember_tabs_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(behavior_grid), remember_tabs_label, 0, row, 1, 1);

    GtkWidget *remember_tabs_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(remember_tabs_switch), settings->remember_open_tabs);
    gtk_widget_set_tooltip_text(remember_tabs_switch, "Restore open tabs when starting the editor");
    g_signal_connect(remember_tabs_switch, "notify::active", G_CALLBACK(on_remember_tabs_toggled), &cb_data);
    gtk_grid_attach(GTK_GRID(behavior_grid), remember_tabs_switch, 1, row, 1, 1);
    row++;

    /* ===== PREVIEW TAB ===== */
    GtkWidget *preview_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(preview_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(preview_scroll), 400);

    GtkWidget *preview_viewport = gtk_viewport_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(preview_scroll), preview_viewport);

    GtkWidget *preview_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(preview_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(preview_grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(preview_grid), 12);
    gtk_widget_set_hexpand(preview_grid, TRUE);
    gtk_widget_set_vexpand(preview_grid, TRUE);
    gtk_container_add(GTK_CONTAINER(preview_viewport), preview_grid);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), preview_scroll, gtk_label_new("🎬 Preview"));

    row = 0;

    /* Shader speed */
    GtkWidget *speed_label = gtk_label_new("Shader Speed:");
    gtk_widget_set_halign(speed_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(preview_grid), speed_label, 0, row, 1, 1);

    GtkWidget *speed_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *speed_spin = gtk_spin_button_new_with_range(0.1, 5.0, 0.1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(speed_spin), 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(speed_spin), settings->shader_speed);
    gtk_widget_set_tooltip_text(speed_spin, "Animation speed multiplier (0.1-5.0)\n1.0 = normal, 2.0 = double, 0.5 = half");
    g_signal_connect(speed_spin, "value-changed", G_CALLBACK(on_shader_speed_changed), &cb_data);
    gtk_box_pack_start(GTK_BOX(speed_box), speed_spin, FALSE, FALSE, 0);

    GtkWidget *reset_speed_btn = gtk_button_new_with_label("Reset");
    gtk_widget_set_tooltip_text(reset_speed_btn, "Reset to 1.0x (normal speed)");
    g_signal_connect(reset_speed_btn, "clicked", G_CALLBACK(on_reset_speed_clicked), speed_spin);
    gtk_box_pack_start(GTK_BOX(speed_box), reset_speed_btn, FALSE, FALSE, 0);

    gtk_grid_attach(GTK_GRID(preview_grid), speed_box, 1, row, 1, 1);
    row++;

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
