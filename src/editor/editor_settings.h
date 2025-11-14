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
    int font_size;
    int tab_width;
    bool auto_compile;
    int preview_fps;
    double shader_speed;
    SplitOrientation split_orientation;
} EditorSettings;

/* Default settings */
#define EDITOR_SETTINGS_DEFAULT { \
    .font_size = 12, \
    .tab_width = 4, \
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