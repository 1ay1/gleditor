/* Error Panel Component - Implementation
 * Displays compilation errors in an expandable panel
 */

#include "editor_error_panel.h"
#include <string.h>
#include <stdlib.h>

/* Module state */
static struct {
    GtkWidget *revealer;
    GtkWidget *text_view;
    GtkTextBuffer *buffer;
    GtkWidget *close_button;
    GtkWidget *error_box;
    bool is_visible;
    bool initialized;
} panel_state = {
    .revealer = NULL,
    .text_view = NULL,
    .buffer = NULL,
    .close_button = NULL,
    .error_box = NULL,
    .is_visible = false,
    .initialized = false
};

/* Close button clicked */
static void on_close_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    editor_error_panel_hide();
}

/* Parse and format error text with syntax highlighting */
static void format_error_text(const char *error_text) {
    if (!panel_state.buffer || !error_text) {
        return;
    }

    /* Clear existing content */
    gtk_text_buffer_set_text(panel_state.buffer, "", 0);

    /* Create text tags for formatting */
    GtkTextTag *error_tag = gtk_text_buffer_create_tag(
        panel_state.buffer, "error",
        "foreground", "#ff4444",
        "weight", PANGO_WEIGHT_BOLD,
        NULL);

    GtkTextTag *warning_tag = gtk_text_buffer_create_tag(
        panel_state.buffer, "warning",
        "foreground", "#ffaa00",
        "weight", PANGO_WEIGHT_BOLD,
        NULL);

    GtkTextTag *line_tag = gtk_text_buffer_create_tag(
        panel_state.buffer, "line",
        "foreground", "#00aaff",
        "weight", PANGO_WEIGHT_BOLD,
        NULL);

    GtkTextTag *mono_tag = gtk_text_buffer_create_tag(
        panel_state.buffer, "mono",
        "family", "monospace",
        NULL);

    /* Get iterator at end */
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(panel_state.buffer, &iter);

    /* Parse and format error text line by line */
    char *text_copy = g_strdup(error_text);
    char *line = strtok(text_copy, "\n");
    int error_count = 0;

    while (line) {
        GtkTextIter start;
        gtk_text_buffer_get_end_iter(panel_state.buffer, &start);

        /* Check for error or warning keywords */
        if (strstr(line, "error") || strstr(line, "ERROR")) {
            /* Insert error line */
            gtk_text_buffer_insert_with_tags(
                panel_state.buffer, &start, "❌ ", -1, error_tag, mono_tag, NULL);
            gtk_text_buffer_get_end_iter(panel_state.buffer, &start);
            gtk_text_buffer_insert_with_tags(
                panel_state.buffer, &start, line, -1, mono_tag, NULL);
            error_count++;
        } else if (strstr(line, "warning") || strstr(line, "WARNING")) {
            /* Insert warning line */
            gtk_text_buffer_insert_with_tags(
                panel_state.buffer, &start, "⚠️  ", -1, warning_tag, mono_tag, NULL);
            gtk_text_buffer_get_end_iter(panel_state.buffer, &start);
            gtk_text_buffer_insert_with_tags(
                panel_state.buffer, &start, line, -1, mono_tag, NULL);
        } else if (strstr(line, ":") && (strstr(line, "(") || strstr(line, "line"))) {
            /* Parse line number references (e.g., "0:42(56)" or "line 42") */
            gtk_text_buffer_insert_with_tags(
                panel_state.buffer, &start, "📍 ", -1, line_tag, mono_tag, NULL);
            gtk_text_buffer_get_end_iter(panel_state.buffer, &start);

            /* Highlight line numbers in the error message */
            char *line_copy = g_strdup(line);
            char *ptr = line_copy;
            char *colon_pos = strchr(ptr, ':');

            while (colon_pos) {
                /* Insert text before colon */
                *colon_pos = '\0';
                gtk_text_buffer_get_end_iter(panel_state.buffer, &start);
                gtk_text_buffer_insert_with_tags(
                    panel_state.buffer, &start, ptr, -1, mono_tag, NULL);

                gtk_text_buffer_get_end_iter(panel_state.buffer, &start);
                gtk_text_buffer_insert_with_tags(
                    panel_state.buffer, &start, ":", -1, line_tag, NULL);

                ptr = colon_pos + 1;

                /* Try to extract line number */
                int line_num;
                if (sscanf(ptr, "%d", &line_num) == 1) {
                    char num_str[32];
                    snprintf(num_str, sizeof(num_str), "%d", line_num);
                    gtk_text_buffer_get_end_iter(panel_state.buffer, &start);
                    gtk_text_buffer_insert_with_tags(
                        panel_state.buffer, &start, num_str, -1, line_tag, mono_tag, NULL);
                    ptr += strlen(num_str);
                }

                colon_pos = strchr(ptr, ':');
            }

            /* Insert remaining text */
            gtk_text_buffer_get_end_iter(panel_state.buffer, &start);
            gtk_text_buffer_insert_with_tags(
                panel_state.buffer, &start, ptr, -1, mono_tag, NULL);

            g_free(line_copy);
        } else {
            /* Insert normal line */
            gtk_text_buffer_insert_with_tags(
                panel_state.buffer, &start, line, -1, mono_tag, NULL);
        }

        /* Add newline */
        gtk_text_buffer_get_end_iter(panel_state.buffer, &start);
        gtk_text_buffer_insert(panel_state.buffer, &start, "\n", -1);

        line = strtok(NULL, "\n");
    }

    g_free(text_copy);

    /* Add summary at the top if errors were found */
    if (error_count > 0) {
        gtk_text_buffer_get_start_iter(panel_state.buffer, &iter);
        char summary[128];
        snprintf(summary, sizeof(summary),
                 "🔴 Compilation Failed - %d error%s found\n\n",
                 error_count, error_count > 1 ? "s" : "");
        gtk_text_buffer_insert_with_tags(
            panel_state.buffer, &iter, summary, -1, error_tag, NULL);
    }
}

/* Public API */

GtkWidget *editor_error_panel_create(void) {
    if (panel_state.initialized) {
        return panel_state.revealer;
    }

    /* Create revealer for smooth show/hide animation */
    panel_state.revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(panel_state.revealer),
                                      GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
    gtk_revealer_set_transition_duration(GTK_REVEALER(panel_state.revealer), 250);
    gtk_revealer_set_reveal_child(GTK_REVEALER(panel_state.revealer), FALSE);

    /* Create main error box */
    panel_state.error_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create header bar */
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(header, 12);
    gtk_widget_set_margin_end(header, 12);
    gtk_widget_set_margin_top(header, 8);
    gtk_widget_set_margin_bottom(header, 8);

    /* Style header with colored background */
    GtkStyleContext *header_context = gtk_widget_get_style_context(header);
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "* { background-color: #3a1f1f; border-bottom: 2px solid #ff4444; }",
        -1, NULL);
    gtk_style_context_add_provider(header_context,
                                   GTK_STYLE_PROVIDER(css_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);

    /* Header label */
    GtkWidget *header_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header_label),
                        "<b>❌ Compilation Errors</b>");
    gtk_widget_set_halign(header_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(header), header_label, TRUE, TRUE, 0);

    /* Close button */
    panel_state.close_button = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_relief(GTK_BUTTON(panel_state.close_button), GTK_RELIEF_NONE);
    g_signal_connect(panel_state.close_button, "clicked", G_CALLBACK(on_close_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(header), panel_state.close_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(panel_state.error_box), header, FALSE, FALSE, 0);

    /* Create scrolled window for error text */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 200);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 150);
    gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(scrolled), 600);
    gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scrolled), TRUE);

    /* Create text view for errors */
    panel_state.text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(panel_state.text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(panel_state.text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(panel_state.text_view), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_margin_start(panel_state.text_view, 12);
    gtk_widget_set_margin_end(panel_state.text_view, 12);
    gtk_widget_set_margin_top(panel_state.text_view, 8);
    gtk_widget_set_margin_bottom(panel_state.text_view, 8);

    /* Get text buffer */
    panel_state.buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(panel_state.text_view));

    /* Style text view */
    GtkStyleContext *text_context = gtk_widget_get_style_context(panel_state.text_view);
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "textview { background-color: #2a2a2a; color: #f0f0f0; font-family: monospace; font-size: 11pt; }",
        -1, NULL);
    gtk_style_context_add_provider(text_context,
                                   GTK_STYLE_PROVIDER(css_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);

    gtk_container_add(GTK_CONTAINER(scrolled), panel_state.text_view);
    gtk_box_pack_start(GTK_BOX(panel_state.error_box), scrolled, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(panel_state.revealer), panel_state.error_box);
    gtk_widget_show_all(panel_state.error_box);

    panel_state.initialized = true;
    panel_state.is_visible = false;

    return panel_state.revealer;
}

void editor_error_panel_show(const char *error_text) {
    if (!panel_state.initialized) {
        return;
    }

    if (error_text && strlen(error_text) > 0) {
        format_error_text(error_text);
        gtk_revealer_set_reveal_child(GTK_REVEALER(panel_state.revealer), TRUE);
        panel_state.is_visible = true;
    }
}

void editor_error_panel_hide(void) {
    if (!panel_state.initialized) {
        return;
    }

    gtk_revealer_set_reveal_child(GTK_REVEALER(panel_state.revealer), FALSE);
    panel_state.is_visible = false;
}

bool editor_error_panel_is_visible(void) {
    return panel_state.is_visible;
}

void editor_error_panel_clear(void) {
    if (panel_state.buffer) {
        gtk_text_buffer_set_text(panel_state.buffer, "", 0);
    }
}

void editor_error_panel_set_text(const char *error_text) {
    if (!panel_state.initialized || !error_text) {
        return;
    }

    format_error_text(error_text);
}

GtkWidget *editor_error_panel_get_widget(void) {
    return panel_state.revealer;
}

void editor_error_panel_destroy(void) {
    if (!panel_state.initialized) {
        return;
    }

    /* Cleanup handled by GTK */
    panel_state.revealer = NULL;
    panel_state.text_view = NULL;
    panel_state.buffer = NULL;
    panel_state.close_button = NULL;
    panel_state.error_box = NULL;
    panel_state.is_visible = false;
    panel_state.initialized = false;
}
