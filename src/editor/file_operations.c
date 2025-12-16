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

/* Helper to run a command and get its output */
static char *run_command(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return NULL;
    }

    char *output = NULL;
    size_t output_size = 0;
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        size_t len = strlen(buffer);
        char *new_output = realloc(output, output_size + len + 1);
        if (!new_output) {
            free(output);
            pclose(fp);
            return NULL;
        }
        output = new_output;
        memcpy(output + output_size, buffer, len + 1);
        output_size += len;
    }

    pclose(fp);
    return output;
}

/* Find shader index in neowall list output */
static int find_shader_index(const char *list_output, const char *shader_filename) {
    if (!list_output || !shader_filename) {
        return -1;
    }
    

    /* Look for [index] shader_filename pattern */
    const char *line = list_output;
    while (line && *line) {
        /* Find line with bracket pattern [N] */
        const char *bracket_start = strstr(line, "[");
        if (bracket_start) {
            const char *bracket_end = strstr(bracket_start, "]");
            if (bracket_end) {
                /* Extract index */
                int index = atoi(bracket_start + 1);

                /* Check if this line contains our shader filename */
                const char *line_end = strchr(line, '\n');
                size_t line_len = line_end ? (size_t)(line_end - line) : strlen(line);

                /* Create a temporary buffer for this line */
                char *line_copy = g_strndup(line, line_len);
                if (line_copy && strstr(line_copy, shader_filename)) {
                    g_free(line_copy);
                    return index;
                }
                g_free(line_copy);
            }
        }

        /* Move to next line */
        const char *next = strchr(line, '\n');
        line = next ? next + 1 : NULL;
    }

    return -1;
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

    /* Construct shader file path and filename */
    /* Strip .glsl extension if already present to avoid double extension */
    char *base_name = g_strdup(shader_name);
    size_t len = strlen(base_name);
    if (len > 5 && strcmp(base_name + len - 5, ".glsl") == 0) {
        base_name[len - 5] = '\0';
    }

    char *shader_filename = g_strdup_printf("%s.glsl", base_name);
    char *shader_path = g_strdup_printf("%s/%s", shader_dir, shader_filename);
    g_free(base_name);
    g_free(shader_dir);

    /* Save shader to NeoWall directory */
    bool success = file_operations_save_file(shader_path, shader_code, error);
    g_free(shader_path);

    if (!success) {
        g_free(shader_filename);
        return false;
    }

    /* Step 1: Kill neowall daemon to reload config */
    system("neowall kill 2>/dev/null");

    /* Give it a moment to stop */
    g_usleep(500000); /* 500ms */

    /* Step 2: Start neowall daemon again */
    system("neowall &");

    /* Wait for daemon to start and load wallpapers */
    g_usleep(2500000); /* 2.5 seconds */

    /* Step 3: Pause cycling to prevent auto-cycle from changing wallpaper */
    system("neowall pause 2>/dev/null");

    /* Step 4: Get the list of wallpapers with retry */
    char *list_output = NULL;
    int index = -1;
    
    /* Retry up to 3 times with increasing delays */
    for (int attempt = 0; attempt < 3 && index < 0; attempt++) {
        if (attempt > 0) {
            g_usleep(1000000); /* Wait 1 second between retries */
        }
        
        list_output = run_command("neowall list 2>/dev/null");
        if (list_output) {
            index = find_shader_index(list_output, shader_filename);
            free(list_output);
        }
    }

    if (index >= 0) {
        /* Step 5: Set the shader as current wallpaper */
        char *set_cmd = g_strdup_printf("neowall set %d", index);
        system(set_cmd);
        g_free(set_cmd);
    }

    /* Step 6: Resume cycling */
    system("neowall resume 2>/dev/null");

    g_free(shader_filename);
    return true;
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
