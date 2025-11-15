/* Tab Manager Module - Header
 * Manages multiple shader tabs with individual state
 */

#ifndef EDITOR_TABS_H
#define EDITOR_TABS_H

#include <gtk/gtk.h>
#include <stdbool.h>

/**
 * Tab information structure
 */
typedef struct {
    int tab_id;                 /* Unique tab identifier */
    char *title;                /* Tab title (filename or "Untitled") */
    char *code;                 /* Shader code */
    char *file_path;            /* File path (NULL if not saved) */
    bool is_modified;           /* Modified flag */
    bool has_compiled;          /* Whether shader has been compiled */
} TabInfo;

/**
 * Callback for when active tab changes
 */
typedef void (*tab_changed_callback_t)(int tab_id, void *user_data);

/**
 * Callback for when tab is closed
 */
typedef bool (*tab_close_callback_t)(int tab_id, void *user_data);

/**
 * Initialize tab manager
 * 
 * @param notebook The GtkNotebook widget to manage tabs in
 * @return true on success
 */
bool editor_tabs_init(GtkNotebook *notebook);

/**
 * Create a new tab
 * 
 * @param title Tab title (NULL for "Untitled")
 * @param code Initial shader code (NULL for empty)
 * @return Tab ID, or -1 on error
 */
int editor_tabs_new(const char *title, const char *code);

/**
 * Close a tab by ID
 * 
 * @param tab_id Tab to close
 * @return true if closed, false if cancelled
 */
bool editor_tabs_close(int tab_id);

/**
 * Close the current active tab
 * 
 * @return true if closed, false if cancelled
 */
bool editor_tabs_close_current(void);

/**
 * Get current active tab ID
 * 
 * @return Tab ID, or -1 if no tabs
 */
int editor_tabs_get_current(void);

/**
 * Switch to a specific tab
 * 
 * @param tab_id Tab to switch to
 * @return true on success
 */
bool editor_tabs_switch_to(int tab_id);

/**
 * Get tab information
 * 
 * @param tab_id Tab ID
 * @return Tab info, or NULL if not found
 */
const TabInfo *editor_tabs_get_info(int tab_id);

/**
 * Update tab code
 * 
 * @param tab_id Tab ID
 * @param code New code
 */
void editor_tabs_set_code(int tab_id, const char *code);

/**
 * Update tab title
 * 
 * @param tab_id Tab ID
 * @param title New title
 */
void editor_tabs_set_title(int tab_id, const char *title);

/**
 * Update tab file path
 * 
 * @param tab_id Tab ID
 * @param file_path New file path (NULL to clear)
 */
void editor_tabs_set_file_path(int tab_id, const char *file_path);

/**
 * Update tab modified flag
 * 
 * @param tab_id Tab ID
 * @param is_modified Modified flag
 */
void editor_tabs_set_modified(int tab_id, bool is_modified);

/**
 * Mark tab as compiled
 * 
 * @param tab_id Tab ID
 * @param compiled Compiled flag
 */
void editor_tabs_set_compiled(int tab_id, bool compiled);

/**
 * Get number of open tabs
 * 
 * @return Number of tabs
 */
int editor_tabs_get_count(void);

/**
 * Set callback for tab changes
 * 
 * @param callback Callback function
 * @param user_data User data to pass to callback
 */
void editor_tabs_set_changed_callback(tab_changed_callback_t callback, void *user_data);

/**
 * Set callback for tab close requests
 * 
 * @param callback Callback function (return false to cancel close)
 * @param user_data User data to pass to callback
 */
void editor_tabs_set_close_callback(tab_close_callback_t callback, void *user_data);

/**
 * Get GtkNotebook widget
 * 
 * @return The notebook widget
 */
GtkWidget *editor_tabs_get_notebook(void);

/**
 * Cleanup tab manager
 */
void editor_tabs_cleanup(void);

#endif /* EDITOR_TABS_H */