/* Text Editor Component - Header
 * Handles the GLSL source code editor with syntax highlighting
 */

#ifndef EDITOR_TEXT_H
#define EDITOR_TEXT_H

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <stdbool.h>
#include "editor_settings.h"

/* Editor configuration */
typedef struct {
    int tab_width;
    int font_size;
    bool auto_compile;
    bool show_line_numbers;
    bool highlight_current_line;
    bool show_minimap;
} editor_text_config_t;

/* Text change callback signature */
typedef void (*editor_text_change_callback_t)(const char *text, gpointer user_data);

/* Cursor move callback signature */
typedef void (*editor_cursor_move_callback_t)(int line, int column, gpointer user_data);

/**
 * Create the text editor widget
 * 
 * @param settings Initial settings (NULL for defaults)
 * @return The text editor widget (GtkScrolledWindow containing GtkSourceView)
 */
GtkWidget *editor_text_create(const EditorSettings *settings);

/**
 * Apply all settings from EditorSettings structure
 * 
 * @param settings Settings to apply
 */
void editor_text_apply_all_settings(const EditorSettings *settings);

/**
 * Get the text editor configuration
 * 
 * @return Current configuration
 */
editor_text_config_t editor_text_get_config(void);

/**
 * Update the text editor configuration
 * 
 * @param config New configuration
 */
void editor_text_set_config(const editor_text_config_t *config);

/**
 * Get the text buffer
 * 
 * @return GtkSourceBuffer containing the shader code
 */
GtkSourceBuffer *editor_text_get_buffer(void);

/**
 * Get the source view widget
 * 
 * @return GtkSourceView widget
 */
GtkWidget *editor_text_get_view(void);

/**
 * Get the current shader code
 * 
 * @return Shader source code (caller must free)
 */
char *editor_text_get_code(void);

/**
 * Set the shader code
 * 
 * @param code Shader source code
 */
void editor_text_set_code(const char *code);

/**
 * Get cursor position
 * 
 * @param line Output: current line (1-based)
 * @param column Output: current column (1-based)
 */
void editor_text_get_cursor_position(int *line, int *column);

/**
 * Set text change callback
 * Called when the text buffer is modified
 * 
 * @param callback Callback function
 * @param user_data User data passed to callback
 */
void editor_text_set_change_callback(editor_text_change_callback_t callback, 
                                     gpointer user_data);

/**
 * Set cursor move callback
 * Called when the cursor position changes
 * 
 * @param callback Callback function
 * @param user_data User data passed to callback
 */
void editor_text_set_cursor_callback(editor_cursor_move_callback_t callback,
                                     gpointer user_data);

/**
 * Apply font size
 * 
 * @param size Font size in points
 */
void editor_text_set_font_size(int size);

/**
 * Apply tab width
 * 
 * @param width Tab width in spaces
 */
void editor_text_set_tab_width(int width);

/**
 * Check if text has been modified
 * 
 * @return true if modified since last save
 */
bool editor_text_is_modified(void);

/**
 * Mark text as saved (clear modified flag)
 */
void editor_text_mark_saved(void);

/**
 * Destroy the text editor and free resources
 */
void editor_text_destroy(void);

#endif /* EDITOR_TEXT_H */