/* File Operations Component - Implementation
 * Handles file dialogs and shader file operations
 */

#include "file_operations.h"
#include "platform_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* NeoWall shader directory (relative to home) */
#ifdef PLATFORM_WINDOWS
    #define NEOWALL_SHADER_DIR "AppData\\Roaming\\neowall\\shaders"
#elif defined(PLATFORM_MACOS)
    #define NEOWALL_SHADER_DIR "Library/Application Support/neowall/shaders"
#else
    #define NEOWALL_SHADER_DIR ".config/neowall/shaders"
#endif

/* Public API */

char *file_operations_load_dialog(GtkWindow *parent) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Load Shader",
        parent,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    /* Add GLSL file filter */
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "GLSL Shaders");
    gtk_file_filter_add_pattern(filter, "*.glsl");
    gtk_file_filter_add_pattern(filter, "*.frag");
    gtk_file_filter_add_pattern(filter, "*.vert");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    /* Add all files filter */
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All Files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    char *filename = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }

    gtk_widget_destroy(dialog);
    return filename;
}

char *file_operations_save_dialog(GtkWindow *parent, const char *current_path) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Save Shader",
        parent,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL
    );

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (current_path) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), current_path);
    } else {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "shader.glsl");
    }

    /* Add GLSL file filter */
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "GLSL Shaders");
    gtk_file_filter_add_pattern(filter, "*.glsl");
    gtk_file_filter_add_pattern(filter, "*.frag");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    char *filename = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }

    gtk_widget_destroy(dialog);
    return filename;
}

char *file_operations_load_file(const char *path, char **error) {
    if (!path) {
        if (error) {
            *error = g_strdup("No file path provided");
        }
        return NULL;
    }

    GError *gerror = NULL;
    char *contents = NULL;
    gsize length = 0;

    if (!g_file_get_contents(path, &contents, &length, &gerror)) {
        if (error) {
            *error = g_strdup(gerror->message);
        }
        g_error_free(gerror);
        return NULL;
    }

    if (error) {
        *error = NULL;
    }

    return contents;
}

bool file_operations_save_file(const char *path, const char *code, char **error) {
    if (!path || !code) {
        if (error) {
            *error = g_strdup("Invalid path or code");
        }
        return false;
    }

    GError *gerror = NULL;
    if (!g_file_set_contents(path, code, -1, &gerror)) {
        if (error) {
            *error = g_strdup(gerror->message);
        }
        g_error_free(gerror);
        return false;
    }

    if (error) {
        *error = NULL;
    }

    return true;
}

char *file_operations_get_neowall_shader_dir(void) {
    const char *home = g_get_home_dir();
    if (!home) {
        return NULL;
    }

    char *dir_path = g_build_filename(home, NEOWALL_SHADER_DIR, NULL);

    /* Create directory if it doesn't exist */
    if (g_mkdir_with_parents(dir_path, 0755) != 0) {
        if (errno != EEXIST) {
            g_free(dir_path);
            return NULL;
        }
    }

    return dir_path;
}

bool file_operations_install_to_neowall(const char *shader_code,
                                        const char *shader_name,
                                        char **error) {
    if (!shader_code || !shader_name) {
        if (error) {
            *error = g_strdup("Invalid shader code or name");
        }
        return false;
    }

    /* Get NeoWall shader directory */
    char *shader_dir = file_operations_get_neowall_shader_dir();
    if (!shader_dir) {
        if (error) {
            *error = g_strdup("Failed to get NeoWall shader directory");
        }
        return false;
    }

    /* Construct shader file path */
    char *shader_path = g_strdup_printf("%s/%s.glsl", shader_dir, shader_name);
    g_free(shader_dir);

    /* Save shader to NeoWall directory */
    bool success = file_operations_save_file(shader_path, shader_code, error);

    g_free(shader_path);
    return success;
}

bool file_operations_file_exists(const char *path) {
    if (!path) {
        return false;
    }

    return g_file_test(path, G_FILE_TEST_EXISTS);
}

const char *file_operations_get_filename(const char *path) {
    if (!path) {
        return NULL;
    }

    const char *filename = strrchr(path, '/');
    if (filename) {
        return filename + 1;
    }

    return path;
}

const char *file_operations_get_extension(const char *path) {
    if (!path) {
        return NULL;
    }

    const char *ext = strrchr(path, '.');
    if (ext && ext != path) {
        return ext + 1;
    }

    return NULL;
}

bool file_operations_confirm_dialog(GtkWindow *parent,
                                    const char *title,
                                    const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "%s", message
    );

    if (title) {
        gtk_window_set_title(GTK_WINDOW(dialog), title);
    }

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return (response == GTK_RESPONSE_YES);
}

void file_operations_error_dialog(GtkWindow *parent,
                                  const char *title,
                                  const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", message
    );

    if (title) {
        gtk_window_set_title(GTK_WINDOW(dialog), title);
    }

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void file_operations_info_dialog(GtkWindow *parent,
                                 const char *title,
                                 const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", message
    );

    if (title) {
        gtk_window_set_title(GTK_WINDOW(dialog), title);
    }

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
