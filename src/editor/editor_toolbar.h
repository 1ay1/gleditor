/* Toolbar Component - Header
 * Handles the editor toolbar with action buttons
 */

#ifndef EDITOR_TOOLBAR_H
#define EDITOR_TOOLBAR_H

#include <gtk/gtk.h>
#include <stdbool.h>

/* View mode options */
typedef enum {
    VIEW_MODE_BOTH = 0,
    VIEW_MODE_EDITOR_ONLY = 1,
    VIEW_MODE_PREVIEW_ONLY = 2
} ViewMode;

/* Toolbar button callback signatures */
typedef void (*editor_toolbar_callback_t)(gpointer user_data);
typedef void (*editor_toolbar_view_callback_t)(ViewMode mode, gpointer user_data);

/**
 * Toolbar button callbacks structure
 */
typedef struct {
    editor_toolbar_callback_t on_new;
    editor_toolbar_callback_t on_load;
    editor_toolbar_callback_t on_save;
    editor_toolbar_callback_t on_compile;
    editor_toolbar_callback_t on_pause;
    editor_toolbar_callback_t on_reset;
    editor_toolbar_callback_t on_install;
    editor_toolbar_callback_t on_settings;
    editor_toolbar_callback_t on_exit;
    editor_toolbar_callback_t on_toggle_split;
    editor_toolbar_view_callback_t on_view_mode_changed;
    gpointer user_data;
} editor_toolbar_callbacks_t;

/**
 * Create the toolbar widget
 * 
 * @param callbacks Button callbacks
 * @return The toolbar widget (GtkBox)
 */
GtkWidget *editor_toolbar_create(const editor_toolbar_callbacks_t *callbacks);

/**
 * Set pause button state
 * 
 * @param paused true for paused state, false for playing
 */
void editor_toolbar_set_paused(bool paused);

/**
 * Get pause button state
 * 
 * @return true if paused, false if playing
 */
bool editor_toolbar_is_paused(void);

/**
 * Set compile button sensitivity
 * 
 * @param sensitive true to enable, false to disable
 */
void editor_toolbar_set_compile_sensitive(bool sensitive);

/**
 * Set compile button visibility
 * 
 * @param visible true to show, false to hide
 */
void editor_toolbar_set_compile_visible(bool visible);

/**
 * Enable or disable install button
 * 
 * @param sensitive true to enable, false to disable
 */
void editor_toolbar_set_install_sensitive(bool sensitive);

/**
 * Get the pause button widget (for external control)
 * 
 * @return The pause toggle button
 */
GtkWidget *editor_toolbar_get_pause_button(void);

/**
 * Update split orientation button icon
 * 
 * @param is_horizontal true for horizontal, false for vertical
 */
void editor_toolbar_set_split_horizontal(bool is_horizontal);

/**
 * Set the active view mode button
 * 
 * @param mode The view mode to activate
 */
void editor_toolbar_set_view_mode(ViewMode mode);

/**
 * Get the current view mode
 * 
 * @return The active view mode
 */
ViewMode editor_toolbar_get_view_mode(void);

/**
 * Destroy the toolbar and free resources
 */
void editor_toolbar_destroy(void);

#endif /* EDITOR_TOOLBAR_H */