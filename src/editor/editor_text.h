/* gleditor - Text Editor Component
 * Handles the code editor with syntax highlighting and features
 */

#ifndef EDITOR_TEXT_H
#define EDITOR_TEXT_H

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <stdbool.h>

/**
 * Text editor context
 */
typedef struct {
    GtkWidget *source_view;
    GtkSourceBuffer *source_buffer;
    GtkWidget *scrolled_window;
    bool modified;
    char *current_file;
    void (*on_modified)(void *user_data);
    void (*on_cursor_moved)(int line, int col, void *user_data);
    void *user_data;
} EditorText;

/**
 * Create text editor component
 * @param font_size Font size in points
 * @param tab_width Tab width in spaces
 * @return EditorText context
 */
EditorText *editor_text_create(int font_size, int tab_width);

/**
 * Destroy text editor component
 * @param editor Text editor context
 */
void editor_text_destroy(EditorText *editor);

/**
 * Get the scrolled window widget
 * @param editor Text editor context
 * @return GtkWidget scrolled window
 */
GtkWidget *editor_text_get_widget(EditorText *editor);

/**
 * Set text content
 * @param editor Text editor context
 * @param text Text to set
 */
void editor_text_set_content(EditorText *editor, const char *text);

/**
 * Get text content
 * @param editor Text editor context
 * @return Allocated string (must be freed)
 */
char *editor_text_get_content(EditorText *editor);

/**
 * Set font size
 * @param editor Text editor context
 * @param size Font size in points
 */
void editor_text_set_font_size(EditorText *editor, int size);

/**
 * Set tab width
 * @param editor Text editor context
 * @param width Tab width in spaces
 */
void editor_text_set_tab_width(EditorText *editor, int width);

/**
 * Check if content is modified
 * @param editor Text editor context
 * @return true if modified
 */
bool editor_text_is_modified(EditorText *editor);

/**
 * Mark content as saved (clear modified flag)
 * @param editor Text editor context
 */
void editor_text_mark_saved(EditorText *editor);

/**
 * Set current file path
 * @param editor Text editor context
 * @param path File path or NULL
 */
void editor_text_set_file_path(EditorText *editor, const char *path);

/**
 * Get current file path
 * @param editor Text editor context
 * @return File path or NULL
 */
const char *editor_text_get_file_path(EditorText *editor);

/**
 * Set modified callback
 * @param editor Text editor context
 * @param callback Callback function
 * @param user_data User data for callback
 */
void editor_text_set_modified_callback(EditorText *editor, 
                                       void (*callback)(void *user_data),
                                       void *user_data);

/**
 * Set cursor moved callback
 * @param editor Text editor context
 * @param callback Callback function (line, col, user_data)
 * @param user_data User data for callback
 */
void editor_text_set_cursor_callback(EditorText *editor,
                                     void (*callback)(int, int, void *),
                                     void *user_data);

#endif /* EDITOR_TEXT_H */