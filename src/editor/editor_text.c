/* Text Editor Component - Implementation
 * Handles the GLSL source code editor with syntax highlighting
 */

#include "editor_text.h"
#include "editor_settings.h"
#include <stdlib.h>
#include <string.h>

/* Suppress GTimeVal deprecation warnings from GtkSourceView headers */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop

/* Module state */
static struct {
    GtkWidget *source_view;
    GtkSourceBuffer *source_buffer;
    GtkWidget *scrolled_window;
    editor_text_config_t config;
    editor_text_change_callback_t change_callback;
    gpointer change_callback_data;
    editor_cursor_move_callback_t cursor_callback;
    gpointer cursor_callback_data;
    bool modified;
    bool initialized;
} editor_state = {
    .source_view = NULL,
    .source_buffer = NULL,
    .scrolled_window = NULL,
    .config = {
        .tab_width = 4,
        .font_size = 11,
        .auto_compile = true,
        .show_line_numbers = true,
        .highlight_current_line = true,
        .show_minimap = false
    },
    .change_callback = NULL,
    .change_callback_data = NULL,
    .cursor_callback = NULL,
    .cursor_callback_data = NULL,
    .modified = false,
    .initialized = false
};

/* Internal callbacks */
static void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
    (void)user_data;

    /* Use the text buffer's built-in modified tracking */
    editor_state.modified = gtk_text_buffer_get_modified(buffer);

    if (editor_state.change_callback) {
        char *text = editor_text_get_code();
        editor_state.change_callback(text, editor_state.change_callback_data);
        g_free(text);
    }
}

static void on_cursor_moved(GtkTextBuffer *buffer, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    (void)user_data;

    if (!editor_state.cursor_callback) {
        return;
    }

    GtkTextIter iter;
    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

    int line = gtk_text_iter_get_line(&iter) + 1;
    int column = gtk_text_iter_get_line_offset(&iter) + 1;

    editor_state.cursor_callback(line, column, editor_state.cursor_callback_data);
}

/* Public API */

GtkWidget *editor_text_create(const EditorSettings *settings) {
    if (editor_state.initialized) {
        g_warning("editor_text_create: Editor already initialized");
        return editor_state.scrolled_window;
    }

    /* Use defaults if no settings provided */
    bool use_settings = (settings != NULL);

    /* Create source buffer with GLSL language */
    GtkSourceLanguageManager *lang_manager = gtk_source_language_manager_get_default();
    GtkSourceLanguage *glsl_lang = gtk_source_language_manager_get_language(lang_manager, "glsl");

    editor_state.source_buffer = gtk_source_buffer_new_with_language(glsl_lang);

    /* Enable syntax highlighting */
    gtk_source_buffer_set_highlight_syntax(editor_state.source_buffer, TRUE);

    /* Set up style scheme */
    GtkSourceStyleSchemeManager *scheme_manager = gtk_source_style_scheme_manager_get_default();
    const char *theme_name = use_settings ? settings->theme : "oblivion";
    GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(scheme_manager, theme_name);
    if (scheme) {
        gtk_source_buffer_set_style_scheme(editor_state.source_buffer, scheme);
    }

    /* Create source view */
    editor_state.source_view = gtk_source_view_new_with_buffer(editor_state.source_buffer);

    /* Configure source view with settings or defaults */
    int tab_width = use_settings ? settings->tab_width : 4;
    int font_size = use_settings ? settings->font_size : 11;
    bool show_line_numbers = use_settings ? settings->show_line_numbers : true;
    bool highlight_current_line = use_settings ? settings->highlight_current_line : true;
    bool show_right_margin = use_settings ? settings->show_right_margin : true;
    bool bracket_matching = use_settings ? settings->bracket_matching : true;
    bool auto_indent = use_settings ? settings->auto_indent : true;
    bool insert_spaces = use_settings ? settings->insert_spaces : true;

    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(editor_state.source_view), show_line_numbers);
    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(editor_state.source_view), highlight_current_line);
    gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(editor_state.source_view), auto_indent);
    gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(editor_state.source_view), tab_width);
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(editor_state.source_view), tab_width);
    gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(editor_state.source_view), insert_spaces);
    gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(editor_state.source_view), show_right_margin);
    gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(editor_state.source_view), 80);
    gtk_source_buffer_set_highlight_matching_brackets(editor_state.source_buffer, bracket_matching);
    gtk_source_view_set_smart_home_end(GTK_SOURCE_VIEW(editor_state.source_view), GTK_SOURCE_SMART_HOME_END_BEFORE);
    gtk_source_view_set_smart_backspace(GTK_SOURCE_VIEW(editor_state.source_view), TRUE);

    /* Set monospace font */
    char font_desc[128];
    const char *font_family = use_settings ? settings->font_family : "Monospace";
    snprintf(font_desc, sizeof(font_desc), "%s %d", font_family, font_size);
    PangoFontDescription *font = pango_font_description_from_string(font_desc);
    gtk_widget_override_font(editor_state.source_view, font);
    pango_font_description_free(font);

    /* Create scrolled window */
    editor_state.scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(editor_state.scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(editor_state.scrolled_window), editor_state.source_view);

    /* Connect signals */
    g_signal_connect(editor_state.source_buffer, "changed",
                     G_CALLBACK(on_buffer_changed), NULL);
    g_signal_connect(editor_state.source_buffer, "notify::cursor-position",
                     G_CALLBACK(on_cursor_moved), NULL);

    editor_state.initialized = true;
    editor_state.modified = false;

    return editor_state.scrolled_window;
}

editor_text_config_t editor_text_get_config(void) {
    return editor_state.config;
}

void editor_text_set_config(const editor_text_config_t *config) {
    if (!config || !editor_state.initialized) {
        return;
    }

    editor_state.config = *config;

    /* Apply configuration to source view */
    if (editor_state.source_view) {
        gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(editor_state.source_view),
                                              config->show_line_numbers);
        gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(editor_state.source_view),
                                                   config->highlight_current_line);
        gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(editor_state.source_view),
                                      config->tab_width);
        gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(editor_state.source_view),
                                         config->tab_width);

        /* Update font size */
        char font_desc[64];
        snprintf(font_desc, sizeof(font_desc), "Monospace %d", config->font_size);
        PangoFontDescription *font = pango_font_description_from_string(font_desc);
        gtk_widget_override_font(editor_state.source_view, font);
        pango_font_description_free(font);
    }
}

void editor_text_apply_all_settings(const EditorSettings *settings) {
    if (!settings || !editor_state.initialized || !editor_state.source_view) {
        return;
    }

    /* Apply theme/color scheme */
    GtkSourceStyleSchemeManager *scheme_manager = gtk_source_style_scheme_manager_get_default();
    GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(scheme_manager, settings->theme);
    if (scheme) {
        gtk_source_buffer_set_style_scheme(editor_state.source_buffer, scheme);
    }

    /* Apply appearance settings */
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(editor_state.source_view),
                                          settings->show_line_numbers);
    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(editor_state.source_view),
                                               settings->highlight_current_line);
    gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(editor_state.source_view),
                                          settings->show_right_margin);
    gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(editor_state.source_view),
                                              settings->right_margin_position);

    /* Apply bracket matching */
    gtk_source_buffer_set_highlight_matching_brackets(editor_state.source_buffer,
                                                      settings->bracket_matching);

    /* Apply whitespace visibility */
    GtkSourceSpaceDrawer *space_drawer = gtk_source_view_get_space_drawer(GTK_SOURCE_VIEW(editor_state.source_view));
    if (settings->show_whitespace) {
        gtk_source_space_drawer_set_types_for_locations(space_drawer,
            GTK_SOURCE_SPACE_LOCATION_ALL,
            GTK_SOURCE_SPACE_TYPE_SPACE | GTK_SOURCE_SPACE_TYPE_TAB | GTK_SOURCE_SPACE_TYPE_NEWLINE);
        gtk_source_space_drawer_set_enable_matrix(space_drawer, TRUE);
    } else {
        gtk_source_space_drawer_set_enable_matrix(space_drawer, FALSE);
    }

    /* Apply word wrap */
    if (settings->word_wrap) {
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(editor_state.source_view), GTK_WRAP_WORD);
    } else {
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(editor_state.source_view), GTK_WRAP_NONE);
    }

    /* Apply behavior settings */
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(editor_state.source_view),
                                  settings->tab_width);
    gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(editor_state.source_view),
                                     settings->tab_width);
    gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(editor_state.source_view),
                                                      settings->insert_spaces);
    gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(editor_state.source_view),
                                    settings->auto_indent);

    if (settings->smart_home_end) {
        gtk_source_view_set_smart_home_end(GTK_SOURCE_VIEW(editor_state.source_view),
                                           GTK_SOURCE_SMART_HOME_END_BEFORE);
    } else {
        gtk_source_view_set_smart_home_end(GTK_SOURCE_VIEW(editor_state.source_view),
                                           GTK_SOURCE_SMART_HOME_END_DISABLED);
    }
    gtk_source_view_set_smart_backspace(GTK_SOURCE_VIEW(editor_state.source_view), TRUE);

    /* Apply font */
    char font_desc[128];
    snprintf(font_desc, sizeof(font_desc), "%s %d", settings->font_family, settings->font_size);
    PangoFontDescription *font = pango_font_description_from_string(font_desc);
    gtk_widget_override_font(editor_state.source_view, font);
    pango_font_description_free(font);
}

GtkSourceBuffer *editor_text_get_buffer(void) {
    return editor_state.source_buffer;
}

GtkWidget *editor_text_get_view(void) {
    return editor_state.source_view;
}

char *editor_text_get_code(void) {
    if (!editor_state.source_buffer) {
        return g_strdup("");
    }

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(editor_state.source_buffer), &start, &end);
    return gtk_text_buffer_get_text(GTK_TEXT_BUFFER(editor_state.source_buffer), &start, &end, FALSE);
}

void editor_text_set_code(const char *code) {
    if (!editor_state.source_buffer) {
        return;
    }

    /* Temporarily disconnect change signal to avoid spurious callbacks */
    g_signal_handlers_block_by_func(editor_state.source_buffer,
                                    G_CALLBACK(on_buffer_changed), NULL);

    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(editor_state.source_buffer), code, -1);

    /* Don't mark as modified here - let the caller decide via mark_saved() */

    /* Unblock signal */
    g_signal_handlers_unblock_by_func(editor_state.source_buffer,
                                      G_CALLBACK(on_buffer_changed), NULL);

    editor_state.modified = false;
}

void editor_text_get_cursor_position(int *line, int *column) {
    if (!editor_state.source_buffer) {
        if (line) *line = 1;
        if (column) *column = 1;
        return;
    }

    GtkTextIter iter;
    GtkTextMark *mark = gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(editor_state.source_buffer));
    gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(editor_state.source_buffer), &iter, mark);

    if (line) {
        *line = gtk_text_iter_get_line(&iter) + 1;
    }
    if (column) {
        *column = gtk_text_iter_get_line_offset(&iter) + 1;
    }
}

void editor_text_set_change_callback(editor_text_change_callback_t callback,
                                     gpointer user_data) {
    editor_state.change_callback = callback;
    editor_state.change_callback_data = user_data;
}

void editor_text_set_cursor_callback(editor_cursor_move_callback_t callback,
                                     gpointer user_data) {
    editor_state.cursor_callback = callback;
    editor_state.cursor_callback_data = user_data;
}

void editor_text_set_font_size(int size) {
    editor_state.config.font_size = size;

    if (editor_state.source_view) {
        char font_desc[64];
        snprintf(font_desc, sizeof(font_desc), "Monospace %d", size);
        PangoFontDescription *font = pango_font_description_from_string(font_desc);
        gtk_widget_override_font(editor_state.source_view, font);
        pango_font_description_free(font);
    }
}

void editor_text_set_tab_width(int width) {
    editor_state.config.tab_width = width;

    if (editor_state.source_view) {
        gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(editor_state.source_view), width);
        gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(editor_state.source_view), width);
    }
}

bool editor_text_is_modified(void) {
    /* Always check the text buffer's modified flag for accuracy */
    if (editor_state.source_buffer) {
        return gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(editor_state.source_buffer));
    }
    return editor_state.modified;
}

void editor_text_mark_saved(void) {
    editor_state.modified = false;
    /* Mark the text buffer as not modified */
    if (editor_state.source_buffer) {
        gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(editor_state.source_buffer), FALSE);
    }
}

void editor_text_destroy(void) {
    if (!editor_state.initialized) {
        return;
    }

    /* Cleanup is handled by GTK when the window is destroyed */
    /* Just reset our state */
    editor_state.source_view = NULL;
    editor_state.source_buffer = NULL;
    editor_state.scrolled_window = NULL;
    editor_state.change_callback = NULL;
    editor_state.change_callback_data = NULL;
    editor_state.cursor_callback = NULL;
    editor_state.cursor_callback_data = NULL;
    editor_state.modified = false;
    editor_state.initialized = false;
}
