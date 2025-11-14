/* Main Window Component - Header
 * Orchestrates all editor components into the main window
 */

#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include <gtk/gtk.h>
#include <stdbool.h>

/**
 * Window configuration
 */
typedef struct {
    int width;
    int height;
    const char *title;
    bool show_menubar;
} editor_window_config_t;

/**
 * Create and show the editor window
 * 
 * @param app GTK application instance (NULL for standalone)
 * @param config Window configuration (NULL for defaults)
 * @return The main window widget
 */
GtkWidget *editor_window_create(GtkApplication *app, const editor_window_config_t *config);

/**
 * Show the editor window
 * Equivalent to editor_window_create with default config
 * 
 * @param app GTK application instance (NULL for standalone)
 */
void editor_window_show(GtkApplication *app);

/**
 * Close the editor window
 */
void editor_window_close(void);

/**
 * Check if editor window is currently open
 * 
 * @return true if window is visible, false otherwise
 */
bool editor_window_is_open(void);

/**
 * Get the main window widget
 * 
 * @return The window widget or NULL if not created
 */
GtkWidget *editor_window_get_widget(void);

/**
 * Set window title
 * 
 * @param title New window title
 */
void editor_window_set_title(const char *title);

/**
 * Update window title with file information
 * 
 * @param filename File name (or NULL for untitled)
 * @param modified true if file is modified
 */
void editor_window_update_title(const char *filename, bool modified);

/**
 * Get current file path
 * 
 * @return Current file path (do not free) or NULL
 */
const char *editor_window_get_current_file(void);

/**
 * Set current file path
 * 
 * @param path New file path (NULL to clear)
 */
void editor_window_set_current_file(const char *path);

/**
 * Check if current file is modified
 * 
 * @return true if modified, false otherwise
 */
bool editor_window_is_modified(void);

/**
 * Set modified state
 * 
 * @param modified true if modified, false otherwise
 */
void editor_window_set_modified(bool modified);

/**
 * Prompt user to save if modified
 * Shows a dialog asking to save unsaved changes
 * 
 * @return true if it's safe to continue (saved or discarded), false if cancelled
 */
bool editor_window_prompt_save_if_modified(void);

/**
 * Load default shader template
 */
void editor_window_load_default_shader(void);

/**
 * Compile current shader
 * 
 * @return true if compilation succeeded, false otherwise
 */
bool editor_window_compile_shader(void);

/**
 * Get shader code from editor
 * 
 * @return Shader code (caller must free)
 */
char *editor_window_get_shader_code(void);

/**
 * Set shader code in editor
 * 
 * @param code Shader code
 */
void editor_window_set_shader_code(const char *code);

/**
 * Toggle pause state
 */
void editor_window_toggle_pause(void);

/**
 * Reset shader animation time
 */
void editor_window_reset_time(void);

/**
 * Destroy the window and all components
 */
void editor_window_destroy(void);

#endif /* EDITOR_WINDOW_H */