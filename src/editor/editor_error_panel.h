/* Error Panel Component - Header
 * Displays compilation errors in an expandable panel
 */

#ifndef EDITOR_ERROR_PANEL_H
#define EDITOR_ERROR_PANEL_H

#include <gtk/gtk.h>
#include <stdbool.h>

/**
 * Create the error panel widget
 * 
 * @return The error panel widget (GtkRevealer containing error display)
 */
GtkWidget *editor_error_panel_create(void);

/**
 * Show error panel with error message
 * 
 * @param error_text The compilation error text to display
 */
void editor_error_panel_show(const char *error_text);

/**
 * Hide the error panel
 */
void editor_error_panel_hide(void);

/**
 * Check if error panel is visible
 * 
 * @return true if visible, false otherwise
 */
bool editor_error_panel_is_visible(void);

/**
 * Clear error panel content
 */
void editor_error_panel_clear(void);

/**
 * Set error text without showing panel
 * 
 * @param error_text The error text to set
 */
void editor_error_panel_set_text(const char *error_text);

/**
 * Get the error panel widget
 * 
 * @return The error panel widget
 */
GtkWidget *editor_error_panel_get_widget(void);

/**
 * Destroy the error panel and free resources
 */
void editor_error_panel_destroy(void);

#endif /* EDITOR_ERROR_PANEL_H */