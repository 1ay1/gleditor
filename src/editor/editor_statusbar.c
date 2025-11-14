/* Status Bar Component - Implementation
 * Handles the editor status bar with FPS, cursor position, and messages
 */

#include "editor_statusbar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Module state */
static struct {
    GtkWidget *statusbar;
    GtkWidget *status_label;
    GtkWidget *status_event_box;
    GtkWidget *fps_label;
    GtkWidget *cursor_label;
    GtkWidget *modified_label;
    editor_statusbar_error_click_callback_t error_click_callback;
    gpointer error_click_user_data;
    bool has_error;
    bool is_modified;
    bool initialized;
} statusbar_state = {
    .statusbar = NULL,
    .status_label = NULL,
    .status_event_box = NULL,
    .fps_label = NULL,
    .cursor_label = NULL,
    .modified_label = NULL,
    .error_click_callback = NULL,
    .error_click_user_data = NULL,
    .has_error = false,
    .is_modified = false,
    .initialized = false
};

/* Error message clicked */
static gboolean on_error_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)event;
    (void)user_data;

    if (statusbar_state.has_error && statusbar_state.error_click_callback) {
        statusbar_state.error_click_callback(statusbar_state.error_click_user_data);
        return TRUE;
    }

    return FALSE;
}

/* Public API */

GtkWidget *editor_statusbar_create(void) {
    if (statusbar_state.initialized) {
        g_warning("editor_statusbar_create: Status bar already initialized");
        return statusbar_state.statusbar;
    }

    /* Create status bar container */
    statusbar_state.statusbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(statusbar_state.statusbar, 12);
    gtk_widget_set_margin_end(statusbar_state.statusbar, 12);
    gtk_widget_set_margin_top(statusbar_state.statusbar, 6);
    gtk_widget_set_margin_bottom(statusbar_state.statusbar, 6);

    /* Status message label (left-aligned, expandable) wrapped in event box for clicking */
    statusbar_state.status_event_box = gtk_event_box_new();
    statusbar_state.status_label = gtk_label_new("");
    gtk_label_set_use_markup(GTK_LABEL(statusbar_state.status_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(statusbar_state.status_label), 0.0);
    gtk_container_add(GTK_CONTAINER(statusbar_state.status_event_box), statusbar_state.status_label);

    /* Connect click event */
    g_signal_connect(statusbar_state.status_event_box, "button-press-event",
                     G_CALLBACK(on_error_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(statusbar_state.statusbar),
                       statusbar_state.status_event_box, TRUE, TRUE, 0);

    /* Modified indicator */
    statusbar_state.modified_label = gtk_label_new("");
    gtk_widget_set_margin_start(statusbar_state.modified_label, 12);
    gtk_box_pack_start(GTK_BOX(statusbar_state.statusbar),
                       statusbar_state.modified_label, FALSE, FALSE, 0);

    /* Cursor position label */
    statusbar_state.cursor_label = gtk_label_new("Line 1, Col 1");
    gtk_widget_set_margin_start(statusbar_state.cursor_label, 12);
    gtk_box_pack_start(GTK_BOX(statusbar_state.statusbar),
                       statusbar_state.cursor_label, FALSE, FALSE, 0);

    /* FPS label (right-aligned) */
    statusbar_state.fps_label = gtk_label_new("");
    gtk_label_set_use_markup(GTK_LABEL(statusbar_state.fps_label), TRUE);
    gtk_widget_set_margin_start(statusbar_state.fps_label, 12);
    gtk_box_pack_start(GTK_BOX(statusbar_state.statusbar),
                       statusbar_state.fps_label, FALSE, FALSE, 0);

    /* Set initial status */
    editor_statusbar_set_message("Ready");
    editor_statusbar_set_fps(0.0);

    statusbar_state.initialized = true;
    statusbar_state.is_modified = false;

    return statusbar_state.statusbar;
}

void editor_statusbar_set_message(const char *message) {
    if (!statusbar_state.status_label || !message) {
        return;
    }

    statusbar_state.has_error = false;

    /* Remove hand cursor */
    gdk_window_set_cursor(gtk_widget_get_window(statusbar_state.status_event_box), NULL);

    /* Use Matrix green color (#00FF41) for status messages */
    char markup[1024];
    snprintf(markup, sizeof(markup),
             "<span foreground='#00FF41'>%s</span>", message);
    gtk_label_set_markup(GTK_LABEL(statusbar_state.status_label), markup);
}

void editor_statusbar_set_fps(double fps) {
    if (!statusbar_state.fps_label) {
        return;
    }

    if (fps > 0.0) {
        char markup[128];
        snprintf(markup, sizeof(markup),
                 "<span foreground='#00FF41'>⚡ FPS: %.0f</span>", fps);
        gtk_label_set_markup(GTK_LABEL(statusbar_state.fps_label), markup);
    } else {
        gtk_label_set_markup(GTK_LABEL(statusbar_state.fps_label), "");
    }
}

void editor_statusbar_set_cursor_position(int line, int column) {
    if (!statusbar_state.cursor_label) {
        return;
    }

    char text[64];
    snprintf(text, sizeof(text), "📍 Line %d, Col %d", line, column);
    gtk_label_set_text(GTK_LABEL(statusbar_state.cursor_label), text);
}

void editor_statusbar_set_modified(bool modified) {
    if (!statusbar_state.modified_label) {
        return;
    }

    statusbar_state.is_modified = modified;

    if (modified) {
        gtk_label_set_markup(GTK_LABEL(statusbar_state.modified_label),
                            "<span foreground='#FF6B6B'>●</span> Modified");
    } else {
        gtk_label_set_text(GTK_LABEL(statusbar_state.modified_label), "");
    }
}

void editor_statusbar_set_error(const char *error) {
    if (!statusbar_state.status_label) {
        return;
    }

    if (error) {
        statusbar_state.has_error = true;

        /* Set hand cursor to indicate clickability */
        if (gtk_widget_get_realized(statusbar_state.status_event_box)) {
            GdkCursor *cursor = gdk_cursor_new_for_display(
                gdk_display_get_default(), GDK_HAND2);
            gdk_window_set_cursor(gtk_widget_get_window(statusbar_state.status_event_box), cursor);
            g_object_unref(cursor);
        }

        char markup[1024];
        snprintf(markup, sizeof(markup),
                 "<span foreground='#FF6B6B'>✗ %s (click to show details)</span>", error);
        gtk_label_set_markup(GTK_LABEL(statusbar_state.status_label), markup);
    } else {
        statusbar_state.has_error = false;
        editor_statusbar_set_message("Ready");
    }
}

GtkWidget *editor_statusbar_get_status_label(void) {
    return statusbar_state.status_label;
}

GtkWidget *editor_statusbar_get_fps_label(void) {
    return statusbar_state.fps_label;
}

GtkWidget *editor_statusbar_get_cursor_label(void) {
    return statusbar_state.cursor_label;
}

void editor_statusbar_set_error_click_callback(editor_statusbar_error_click_callback_t callback, gpointer user_data) {
    statusbar_state.error_click_callback = callback;
    statusbar_state.error_click_user_data = user_data;
}

void editor_statusbar_destroy(void) {
    if (!statusbar_state.initialized) {
        return;
    }

    /* Cleanup handled by GTK */
    statusbar_state.statusbar = NULL;
    statusbar_state.status_label = NULL;
    statusbar_state.status_event_box = NULL;
    statusbar_state.fps_label = NULL;
    statusbar_state.cursor_label = NULL;
    statusbar_state.modified_label = NULL;
    statusbar_state.error_click_callback = NULL;
    statusbar_state.error_click_user_data = NULL;
    statusbar_state.has_error = false;
    statusbar_state.is_modified = false;
    statusbar_state.initialized = false;
}
