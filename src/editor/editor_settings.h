/* gleditor - Settings Management
 * Handles editor settings persistence and dialog
 */

#ifndef EDITOR_SETTINGS_H
#define EDITOR_SETTINGS_H

#include <gtk/gtk.h>
#include <stdbool.h>

/* Split orientation options */
typedef enum {
    SPLIT_HORIZONTAL = 0,
    SPLIT_VERTICAL = 1
} SplitOrientation;

/* Editor settings structure */
typedef struct {
    /* Editor appearance */
    char font_family[64];
    int font_size;
    char theme[32];
    bool show_line_numbers;
    bool highlight_current_line;
    bool show_right_margin;
    int right_margin_position;
    bool show_whitespace;
    bool word_wrap;
    
    /* Editor behavior */
    int tab_width;
    bool insert_spaces;
    bool auto_indent;
    bool smart_home_end;
    bool bracket_matching;
    
    /* Compilation */
    bool auto_compile;
    
    /* Preview */
    int preview_fps;
    double shader_speed;
    
    /* Layout */
    SplitOrientation split_orientation;
} EditorSettings;

/* Default settings */
#define EDITOR_SETTINGS_DEFAULT { \
    .font_family = "Monospace", \
    .font_size = 11, \
    .theme = "oblivion", \
    .show_line_numbers = true, \
    .highlight_current_line = true, \
    .show_right_margin = true, \
    .right_margin_position = 80, \
    .show_whitespace = false, \
    .word_wrap = false, \
    .tab_width = 4, \
    .insert_spaces = true, \
    .auto_indent = true, \
    .smart_home_end = true, \
    .bracket_matching = true, \
    .auto_compile = true, \
    .preview_fps = 60, \
    .shader_speed = 1.0, \
    .split_orientation = SPLIT_HORIZONTAL \
}

/**
 * Load settings from config file
 * @param settings Settings structure to populate
 */
void editor_settings_load(EditorSettings *settings);

/**
 * Save settings to config file
 * @param settings Settings to save
 */
void editor_settings_save(const EditorSettings *settings);

/**
 * Show settings dialog
 * @param parent Parent window
 * @param settings Current settings
 * @param on_change Callback when settings change
 * @param user_data User data for callback
 */
void editor_settings_show_dialog(GtkWindow *parent, 
                                  EditorSettings *settings,
                                  void (*on_change)(EditorSettings *, gpointer),
                                  gpointer user_data);

#endif /* EDITOR_SETTINGS_H */