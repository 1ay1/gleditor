/* Tab Manager Module - Implementation
 * Manages multiple shader tabs with individual state
 */

#include "editor_tabs.h"
#include "platform_compat.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/* Maximum number of tabs */
#define MAX_TABS 20

/* Tab state structure */
typedef struct {
    int tab_id;
    GtkWidget *label_box;
    GtkWidget *label;
    GtkWidget *close_button;
    char *title;
    char *code;
    char *file_path;
    bool is_modified;
    bool has_compiled;
    bool is_active;
} Tab;

/* Module state */
static struct {
    GtkNotebook *notebook;
    Tab tabs[MAX_TABS];
    int tab_count;
    int next_tab_id;
    int current_tab_id;
    tab_changed_callback_t changed_callback;
    void *changed_callback_data;
    tab_close_callback_t close_callback;
    void *close_callback_data;
    bool initialized;
} state = {
    .notebook = NULL,
    .tab_count = 0,
    .next_tab_id = 1,
    .current_tab_id = -1,
    .changed_callback = NULL,
    .changed_callback_data = NULL,
    .close_callback = NULL,
    .close_callback_data = NULL,
    .initialized = false
};

/* Forward declarations */
static void on_tab_close_clicked(GtkButton *button, gpointer user_data);
static void on_tab_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data);
static Tab *find_tab_by_id(int tab_id);
static Tab *find_tab_by_page_num(int page_num);
static void update_tab_label(Tab *tab);

/* Helper: Find tab by ID */
static Tab *find_tab_by_id(int tab_id) {
    for (int i = 0; i < state.tab_count; i++) {
        if (state.tabs[i].tab_id == tab_id) {
            return &state.tabs[i];
        }
    }
    return NULL;
}

/* Helper: Find tab by notebook page number */
static Tab *find_tab_by_page_num(int page_num) {
    if (page_num < 0 || page_num >= state.tab_count) {
        return NULL;
    }

    /* Page numbers map directly to tab array indices */
    return &state.tabs[page_num];
}

/* Helper: Get tab index by ID */
static int get_tab_index(int tab_id) {
    for (int i = 0; i < state.tab_count; i++) {
        if (state.tabs[i].tab_id == tab_id) {
            return i;
        }
    }
    return -1;
}

/* Helper: Update tab label with title and modified indicator */
static void update_tab_label(Tab *tab) {
    if (!tab || !tab->label) return;

    char *label_text;
    if (tab->is_modified) {
        label_text = g_strdup_printf("● %s", tab->title);
    } else {
        label_text = g_strdup(tab->title);
    }

    gtk_label_set_text(GTK_LABEL(tab->label), label_text);
    g_free(label_text);

    /* Set tooltip with full file path if available */
    if (tab->file_path) {
        gtk_widget_set_tooltip_text(tab->label_box, tab->file_path);
    } else {
        gtk_widget_set_tooltip_text(tab->label_box, "Unsaved shader");
    }
}

/* Callback: Tab close button clicked */
static void on_tab_close_clicked(GtkButton *button, gpointer user_data) {
    (void)button;

    int tab_id = GPOINTER_TO_INT(user_data);
    editor_tabs_close(tab_id);
}

/* Callback: Tab switched */
static void on_tab_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
    (void)notebook;
    (void)page;
    (void)user_data;

    Tab *tab = find_tab_by_page_num(page_num);
    if (!tab) return;

    /* Update current tab */
    state.current_tab_id = tab->tab_id;

    /* Mark all tabs as inactive except this one */
    for (int i = 0; i < state.tab_count; i++) {
        state.tabs[i].is_active = (state.tabs[i].tab_id == tab->tab_id);
    }

    /* Notify callback */
    if (state.changed_callback) {
        state.changed_callback(tab->tab_id, state.changed_callback_data);
    }
}

/* Public API Implementation */

bool editor_tabs_init(GtkNotebook *notebook) {
    if (!notebook) return false;
    if (state.initialized) return false;

    state.notebook = notebook;
    state.tab_count = 0;
    state.next_tab_id = 1;
    state.current_tab_id = -1;

    /* Initialize tabs array */
    memset(state.tabs, 0, sizeof(state.tabs));

    /* Configure notebook */
    gtk_notebook_set_scrollable(notebook, TRUE);
    gtk_notebook_set_show_border(notebook, FALSE);

    /* Connect signals */
    g_signal_connect(notebook, "switch-page", G_CALLBACK(on_tab_switched), NULL);

    state.initialized = true;
    return true;
}

int editor_tabs_new(const char *title, const char *code) {
    if (!state.initialized) return -1;
    if (state.tab_count >= MAX_TABS) return -1;

    /* Create new tab */
    Tab *tab = &state.tabs[state.tab_count];
    tab->tab_id = state.next_tab_id++;
    tab->title = g_strdup(title ? title : "Untitled");
    tab->code = g_strdup(code ? code : "");
    tab->file_path = NULL;
    tab->is_modified = false;
    tab->has_compiled = false;
    tab->is_active = false;

    /* Create tab label with close button */
    tab->label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    tab->label = gtk_label_new(tab->title);
    gtk_box_pack_start(GTK_BOX(tab->label_box), tab->label, FALSE, FALSE, 0);

    tab->close_button = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_relief(GTK_BUTTON(tab->close_button), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(tab->close_button, "Close tab");
    g_signal_connect(tab->close_button, "clicked",
                     G_CALLBACK(on_tab_close_clicked),
                     GINT_TO_POINTER(tab->tab_id));
    gtk_box_pack_start(GTK_BOX(tab->label_box), tab->close_button, FALSE, FALSE, 0);

    gtk_widget_show_all(tab->label_box);

    /* Create placeholder content (empty box) */
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(content);

    /* Add to notebook */
    int page_num = gtk_notebook_append_page(state.notebook, content, tab->label_box);
    gtk_notebook_set_tab_reorderable(state.notebook, content, TRUE);

    state.tab_count++;

    /* Switch to new tab */
    gtk_notebook_set_current_page(state.notebook, page_num);

    /* Set as current tab (GTK doesn't fire switch-page for first tab) */
    state.current_tab_id = tab->tab_id;
    tab->is_active = true;

    return tab->tab_id;
}

bool editor_tabs_close(int tab_id) {
    Tab *tab = find_tab_by_id(tab_id);
    if (!tab) return false;

    /* Call close callback to check if we should cancel */
    if (state.close_callback) {
        if (!state.close_callback(tab_id, state.close_callback_data)) {
            return false; /* Close cancelled */
        }
    }

    /* Get page number */
    int page_num = get_tab_index(tab_id);
    if (page_num < 0) return false;

    /* Remove from notebook */
    gtk_notebook_remove_page(state.notebook, page_num);

    /* Free tab data and NULL pointers to prevent double-free */
    if (tab->title) {
        g_free(tab->title);
        tab->title = NULL;
    }
    if (tab->code) {
        g_free(tab->code);
        tab->code = NULL;
    }
    if (tab->file_path) {
        g_free(tab->file_path);
        tab->file_path = NULL;
    }

    /* Shift remaining tabs down */
    for (int i = page_num; i < state.tab_count - 1; i++) {
        state.tabs[i] = state.tabs[i + 1];
    }

    state.tab_count--;

    /* Clear last tab slot */
    memset(&state.tabs[state.tab_count], 0, sizeof(Tab));

    /* Update current tab ID */
    if (state.tab_count > 0) {
        int current_page = gtk_notebook_get_current_page(state.notebook);
        Tab *current_tab = find_tab_by_page_num(current_page);
        if (current_tab) {
            state.current_tab_id = current_tab->tab_id;
        }
    } else {
        state.current_tab_id = -1;
    }

    return true;
}

bool editor_tabs_close_current(void) {
    if (state.current_tab_id < 0) return false;
    return editor_tabs_close(state.current_tab_id);
}

int editor_tabs_get_current(void) {
    return state.current_tab_id;
}

bool editor_tabs_switch_to(int tab_id) {
    int index = get_tab_index(tab_id);
    if (index < 0) return false;

    gtk_notebook_set_current_page(state.notebook, index);
    return true;
}

const TabInfo *editor_tabs_get_info(int tab_id) {
    Tab *tab = find_tab_by_id(tab_id);
    if (!tab) return NULL;

    /* Return tab info (note: this is a static structure, caller should not free) */
    static TabInfo info;
    info.tab_id = tab->tab_id;
    info.title = tab->title;
    info.code = tab->code;
    info.file_path = tab->file_path;
    info.is_modified = tab->is_modified;
    info.has_compiled = tab->has_compiled;

    return &info;
}

void editor_tabs_set_code(int tab_id, const char *code) {
    Tab *tab = find_tab_by_id(tab_id);
    if (!tab) return;

    if (tab->code) g_free(tab->code);
    tab->code = g_strdup(code ? code : "");
}

void editor_tabs_set_title(int tab_id, const char *title) {
    Tab *tab = find_tab_by_id(tab_id);
    if (!tab) return;

    if (tab->title) g_free(tab->title);
    tab->title = g_strdup(title ? title : "Untitled");

    update_tab_label(tab);
}

void editor_tabs_set_file_path(int tab_id, const char *file_path) {
    Tab *tab = find_tab_by_id(tab_id);
    if (!tab) return;

    if (tab->file_path) {
        g_free(tab->file_path);
        tab->file_path = NULL;
    }

    if (file_path) {
        tab->file_path = g_strdup(file_path);

        /* Update title to filename */
        char *basename = g_path_get_basename(file_path);
        editor_tabs_set_title(tab_id, basename);
        g_free(basename);
    }

    update_tab_label(tab);
}

void editor_tabs_set_modified(int tab_id, bool is_modified) {
    Tab *tab = find_tab_by_id(tab_id);
    if (!tab) return;

    tab->is_modified = is_modified;
    update_tab_label(tab);
}

void editor_tabs_set_compiled(int tab_id, bool compiled) {
    Tab *tab = find_tab_by_id(tab_id);
    if (!tab) return;

    tab->has_compiled = compiled;
}

int editor_tabs_get_count(void) {
    return state.tab_count;
}

void editor_tabs_set_changed_callback(tab_changed_callback_t callback, void *user_data) {
    state.changed_callback = callback;
    state.changed_callback_data = user_data;
}

void editor_tabs_set_close_callback(tab_close_callback_t callback, void *user_data) {
    state.close_callback = callback;
    state.close_callback_data = user_data;
}

GtkWidget *editor_tabs_get_notebook(void) {
    return GTK_WIDGET(state.notebook);
}

void editor_tabs_cleanup(void) {
    if (!state.initialized) return;

    /* Free all tab data - check for NULL to avoid double-free */
    for (int i = 0; i < state.tab_count; i++) {
        if (state.tabs[i].title) {
            g_free(state.tabs[i].title);
            state.tabs[i].title = NULL;
        }
        if (state.tabs[i].code) {
            g_free(state.tabs[i].code);
            state.tabs[i].code = NULL;
        }
        if (state.tabs[i].file_path) {
            g_free(state.tabs[i].file_path);
            state.tabs[i].file_path = NULL;
        }
    }

    state.tab_count = 0;
    state.current_tab_id = -1;
    state.initialized = false;
}

bool editor_tabs_save_session(void) {
    if (!state.initialized) return false;

    /* Get config directory */
    char config_dir[PATH_MAX];
    platform_get_config_dir(config_dir, sizeof(config_dir));

    /* Create config directory if it doesn't exist */
    platform_mkdir_recursive(config_dir);

    char session_path[PATH_MAX];
    platform_path_join(session_path, sizeof(session_path), config_dir, "tabs_session.ini");

    char *session_file = g_strdup(session_path);

    GKeyFile *keyfile = g_key_file_new();

    /* Save each tab */
    for (int i = 0; i < state.tab_count; i++) {
        Tab *tab = &state.tabs[i];
        char group[32];
        snprintf(group, sizeof(group), "Tab%d", i);

        /* Save basic info */
        g_key_file_set_string(keyfile, group, "title", tab->title);
        g_key_file_set_boolean(keyfile, group, "is_active", tab->is_active);
        g_key_file_set_boolean(keyfile, group, "is_modified", tab->is_modified);

        /* Save file path if it exists */
        if (tab->file_path) {
            g_key_file_set_string(keyfile, group, "file_path", tab->file_path);
        }

        /* Save code (always save, even if file exists, in case of unsaved changes) */
        if (tab->code) {
            g_key_file_set_string(keyfile, group, "code", tab->code);
        }
    }

    /* Save tab count */
    g_key_file_set_integer(keyfile, "Session", "tab_count", state.tab_count);
    g_key_file_set_integer(keyfile, "Session", "current_tab_id", state.current_tab_id);

    /* Write to file */
    GError *error = NULL;
    gboolean success = g_key_file_save_to_file(keyfile, session_file, &error);

    if (!success && error) {
        g_warning("Failed to save tab session: %s", error->message);
        g_error_free(error);
    }

    g_key_file_free(keyfile);
    g_free(session_file);

    return success;
}

bool editor_tabs_restore_session(void) {
    if (!state.initialized) {
        return false;
    }

    /* Get config directory */
    char config_dir[PATH_MAX];
    platform_get_config_dir(config_dir, sizeof(config_dir));

    char session_path[PATH_MAX];
    platform_path_join(session_path, sizeof(session_path), config_dir, "tabs_session.ini");

    char *session_file = g_strdup(session_path);

    /* Check if session file exists */
    if (!g_file_test(session_file, G_FILE_TEST_EXISTS)) {
        g_free(session_file);
        return false;
    }

    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;

    if (!g_key_file_load_from_file(keyfile, session_file, G_KEY_FILE_NONE, &error)) {
        g_warning("Failed to load tab session: %s", error ? error->message : "unknown error");
        if (error) g_error_free(error);
        g_key_file_free(keyfile);
        g_free(session_file);
        return false;
    }

    /* Get tab count */
    int tab_count = g_key_file_get_integer(keyfile, "Session", "tab_count", &error);
    if (error) {
        g_error_free(error);
        error = NULL;
        tab_count = 0;
    }

    int saved_current_tab_id = g_key_file_get_integer(keyfile, "Session", "current_tab_id", &error);
    (void)saved_current_tab_id; /* Currently unused, tab restoration uses is_active flag */
    if (error) {
        g_error_free(error);
        error = NULL;
    }

    /* Restore each tab */
    int restored_current_index = -1;
    for (int i = 0; i < tab_count && i < MAX_TABS; i++) {
        char group[32];
        snprintf(group, sizeof(group), "Tab%d", i);

        /* Get tab info */
        char *title = g_key_file_get_string(keyfile, group, "title", NULL);
        char *file_path = g_key_file_get_string(keyfile, group, "file_path", NULL);
        char *code = g_key_file_get_string(keyfile, group, "code", NULL);
        gboolean was_active = g_key_file_get_boolean(keyfile, group, "is_active", NULL);

        /* Create tab */
        int tab_id = -1;
        if (file_path && g_file_test(file_path, G_FILE_TEST_EXISTS)) {
            /* File still exists, load from file */
            GError *file_error = NULL;
            char *file_contents = NULL;
            if (g_file_get_contents(file_path, &file_contents, NULL, &file_error)) {
                tab_id = editor_tabs_new(title, file_contents);
                if (tab_id >= 0) {
                    editor_tabs_set_file_path(tab_id, file_path);
                    editor_tabs_set_modified(tab_id, false);
                }
                g_free(file_contents);
            } else {
                /* File couldn't be read, use saved code */
                tab_id = editor_tabs_new(title, code);
                if (file_error) g_error_free(file_error);
            }
        } else if (code) {
            /* No file path or file doesn't exist, use saved code */
            tab_id = editor_tabs_new(title, code);
        }

        /* Track which tab should be active */
        if (was_active && tab_id >= 0) {
            restored_current_index = state.tab_count - 1;
        }

        g_free(title);
        g_free(file_path);
        g_free(code);
    }

    /* Switch to previously active tab */
    if (restored_current_index >= 0 && restored_current_index < state.tab_count) {
        gtk_notebook_set_current_page(state.notebook, restored_current_index);
    }

    g_key_file_free(keyfile);
    g_free(session_file);

    return state.tab_count > 0;
}
