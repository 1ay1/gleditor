/* Keyboard Shortcuts Component - Implementation
 * Handles all keyboard shortcuts for the editor
 */

#include "keyboard_shortcuts.h"
#include <gdk/gdkkeysyms.h>

/* Module state */
static struct {
    keyboard_shortcuts_callbacks_t callbacks;
    gboolean initialized;
} shortcuts_state = {
    .callbacks = {0},
    .initialized = FALSE
};

/* Key press event handler */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    if (!shortcuts_state.initialized) {
        return FALSE;
    }

    gboolean ctrl = (event->state & GDK_CONTROL_MASK) != 0;
    gboolean shift = (event->state & GDK_SHIFT_MASK) != 0;
    gboolean alt = (event->state & GDK_MOD1_MASK) != 0;

    /* Prevent handling if only modifier key is pressed */
    if (event->keyval == GDK_KEY_Control_L || event->keyval == GDK_KEY_Control_R ||
        event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_R ||
        event->keyval == GDK_KEY_Alt_L || event->keyval == GDK_KEY_Alt_R) {
        return FALSE;
    }

    /* ===== FUNCTION KEYS ===== */

    /* F1 - Help */
    if (event->keyval == GDK_KEY_F1) {
        if (shortcuts_state.callbacks.on_help) {
            shortcuts_state.callbacks.on_help(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* F5 - Compile Shader */
    if (event->keyval == GDK_KEY_F5) {
        if (shortcuts_state.callbacks.on_compile) {
            shortcuts_state.callbacks.on_compile(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* F6 - Toggle Split Orientation */
    if (event->keyval == GDK_KEY_F6) {
        if (shortcuts_state.callbacks.on_toggle_split) {
            shortcuts_state.callbacks.on_toggle_split(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* F7 - Show Only Editor */
    if (event->keyval == GDK_KEY_F7) {
        if (shortcuts_state.callbacks.on_view_mode_changed) {
            shortcuts_state.callbacks.on_view_mode_changed(VIEW_MODE_EDITOR_ONLY,
                                                          shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* F8 - Show Only Preview */
    if (event->keyval == GDK_KEY_F8) {
        if (shortcuts_state.callbacks.on_view_mode_changed) {
            shortcuts_state.callbacks.on_view_mode_changed(VIEW_MODE_PREVIEW_ONLY,
                                                          shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* F9 - Show Both Panels */
    if (event->keyval == GDK_KEY_F9) {
        if (shortcuts_state.callbacks.on_view_mode_changed) {
            shortcuts_state.callbacks.on_view_mode_changed(VIEW_MODE_BOTH,
                                                          shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* ===== FILE OPERATIONS (Ctrl+...) ===== */

    /* Ctrl+N - New File */
    if (ctrl && !shift && !alt && event->keyval == GDK_KEY_n) {
        if (shortcuts_state.callbacks.on_new) {
            shortcuts_state.callbacks.on_new(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* Ctrl+O - Open File */
    if (ctrl && !shift && !alt && event->keyval == GDK_KEY_o) {
        if (shortcuts_state.callbacks.on_open) {
            shortcuts_state.callbacks.on_open(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* Ctrl+S - Save File */
    if (ctrl && !shift && !alt && event->keyval == GDK_KEY_s) {
        if (shortcuts_state.callbacks.on_save) {
            shortcuts_state.callbacks.on_save(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* Ctrl+Shift+S - Save As */
    if (ctrl && shift && !alt && event->keyval == GDK_KEY_S) {
        if (shortcuts_state.callbacks.on_save_as) {
            shortcuts_state.callbacks.on_save_as(shortcuts_state.callbacks.user_data);
        } else if (shortcuts_state.callbacks.on_save) {
            /* Fallback to regular save if save_as not implemented */
            shortcuts_state.callbacks.on_save(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* Ctrl+W - Close */
    if (ctrl && !shift && !alt && event->keyval == GDK_KEY_w) {
        if (shortcuts_state.callbacks.on_close) {
            shortcuts_state.callbacks.on_close(shortcuts_state.callbacks.user_data);
        } else if (shortcuts_state.callbacks.on_new) {
            /* Fallback to new if close not implemented */
            shortcuts_state.callbacks.on_new(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* Ctrl+Q - Exit */
    if (ctrl && !shift && !alt && event->keyval == GDK_KEY_q) {
        if (shortcuts_state.callbacks.on_exit) {
            shortcuts_state.callbacks.on_exit(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* ===== COMPILATION & DEBUGGING ===== */

    /* Ctrl+B - Compile Shader (alternative to F5) */
    if (ctrl && !shift && !alt && event->keyval == GDK_KEY_b) {
        if (shortcuts_state.callbacks.on_compile) {
            shortcuts_state.callbacks.on_compile(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* Ctrl+E - Toggle Error Panel */
    if (ctrl && !shift && !alt && event->keyval == GDK_KEY_e) {
        if (shortcuts_state.callbacks.on_toggle_error_panel) {
            shortcuts_state.callbacks.on_toggle_error_panel(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* ===== SETTINGS & HELP ===== */

    /* Ctrl+, - Settings */
    if (ctrl && !shift && !alt && event->keyval == GDK_KEY_comma) {
        if (shortcuts_state.callbacks.on_settings) {
            shortcuts_state.callbacks.on_settings(shortcuts_state.callbacks.user_data);
        }
        return TRUE;
    }

    /* Let other handlers process the event */
    return FALSE;
}

/* Public API */
gboolean keyboard_shortcuts_init(GtkWidget *window, const keyboard_shortcuts_callbacks_t *callbacks) {
    if (!window || !callbacks) {
        g_warning("keyboard_shortcuts_init: Invalid parameters");
        return FALSE;
    }

    if (shortcuts_state.initialized) {
        g_warning("keyboard_shortcuts_init: Already initialized");
        return FALSE;
    }

    /* Copy callbacks */
    shortcuts_state.callbacks = *callbacks;

    /* Connect key press event to window */
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    shortcuts_state.initialized = TRUE;
    return TRUE;
}

void keyboard_shortcuts_get_help_text(void) {
    /* This function is intentionally empty */
    /* The help text is defined in editor_help.c */
    /* This function exists for API completeness only */
}
