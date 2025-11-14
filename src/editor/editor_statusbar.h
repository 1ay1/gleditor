/* Status Bar Component - Header
 * Handles the editor status bar with FPS, cursor position, and messages
 */

#ifndef EDITOR_STATUSBAR_H
#define EDITOR_STATUSBAR_H

#include <gtk/gtk.h>
#include <stdbool.h>

/* Error click callback signature */
typedef void (*editor_statusbar_error_click_callback_t)(gpointer user_data);

/**
 * Create the status bar widget
 * 
 * @return The status bar widget
 */
GtkWidget *editor_statusbar_create(void);

/**
 * Update status message
 * 
 * @param message Status message (supports Pango markup)
 */
void editor_statusbar_set_message(const char *message);

/**
 * Update FPS display
 * 
 * @param fps Frames per second
 */
void editor_statusbar_set_fps(double fps);

/**
 * Update cursor position display
 * 
 * @param line Current line number (1-based)
 * @param column Current column number (1-based)
 */
void editor_statusbar_set_cursor_position(int line, int column);

/**
 * Set file modified indicator
 * 
 * @param modified true if file is modified, false otherwise
 */
void editor_statusbar_set_modified(bool modified);

/**
 * Show error message in status bar
 * 
 * @param error Error message (NULL to clear)
 */
void editor_statusbar_set_error(const char *error);

/**
 * Get the status label widget (for direct access if needed)
 * 
 * @return The status label widget
 */
GtkWidget *editor_statusbar_get_status_label(void);

/**
 * Get the FPS label widget (for direct access if needed)
 * 
 * @return The FPS label widget
 */
GtkWidget *editor_statusbar_get_fps_label(void);

/**
 * Get the cursor label widget (for direct access if needed)
 * 
 * @return The cursor position label widget
 */
GtkWidget *editor_statusbar_get_cursor_label(void);

/**
 * Set callback for when error message is clicked
 * 
 * @param callback Callback function
 * @param user_data User data for callback
 */
void editor_statusbar_set_error_click_callback(editor_statusbar_error_click_callback_t callback, gpointer user_data);

/**
 * Destroy the status bar and free resources
 */
void editor_statusbar_destroy(void);

#endif /* EDITOR_STATUSBAR_H */