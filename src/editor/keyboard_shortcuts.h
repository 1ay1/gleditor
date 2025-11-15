/* Keyboard Shortcuts Component - Header
 * Handles all keyboard shortcuts for the editor
 */

#ifndef KEYBOARD_SHORTCUTS_H
#define KEYBOARD_SHORTCUTS_H

#include <gtk/gtk.h>
#include "editor_toolbar.h"

/**
 * Keyboard shortcut callbacks structure
 * Contains function pointers for all shortcut actions
 */
typedef struct {
    /* File operations */
    void (*on_new)(gpointer user_data);
    void (*on_open)(gpointer user_data);
    void (*on_save)(gpointer user_data);
    void (*on_save_as)(gpointer user_data);
    void (*on_close)(gpointer user_data);
    void (*on_exit)(gpointer user_data);

    /* Editing & Compilation */
    void (*on_compile)(gpointer user_data);
    void (*on_toggle_error_panel)(gpointer user_data);

    /* View & Navigation */
    void (*on_toggle_split)(gpointer user_data);
    void (*on_view_mode_changed)(ViewMode mode, gpointer user_data);
    void (*on_settings)(gpointer user_data);
    void (*on_help)(gpointer user_data);

    /* User data for callbacks */
    gpointer user_data;
} keyboard_shortcuts_callbacks_t;

/**
 * Initialize keyboard shortcuts handler
 * 
 * @param window The main window to attach key handler to
 * @param callbacks Callback functions for shortcut actions
 * @return TRUE if initialization was successful
 */
gboolean keyboard_shortcuts_init(GtkWidget *window, const keyboard_shortcuts_callbacks_t *callbacks);

/**
 * Get list of all keyboard shortcuts for display in help
 * This is for documentation purposes only - actual shortcuts are handled internally
 */
void keyboard_shortcuts_get_help_text(void);

#endif /* KEYBOARD_SHORTCUTS_H */