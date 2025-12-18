/* File Operations Component - Implementation
 * Handles file dialogs and shader file operations
 */

#include "file_operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        if (error) *error = g_strdup("No file path provided");
        return NULL;
    }

    GError *gerror = NULL;
    char *contents = NULL;
    gsize length = 0;

    if (!g_file_get_contents(path, &contents, &length, &gerror)) {
        if (error) *error = g_strdup(gerror->message);
        g_error_free(gerror);
        return NULL;
    }

    if (error) *error = NULL;
    return contents;
}

bool file_operations_save_file(const char *path, const char *code, char **error) {
    if (!path || !code) {
        if (error) *error = g_strdup("Invalid path or code");
        return false;
    }

    GError *gerror = NULL;
    if (!g_file_set_contents(path, code, -1, &gerror)) {
        if (error) *error = g_strdup(gerror->message);
        g_error_free(gerror);
        return false;
    }

    if (error) *error = NULL;
    return true;
}

char *file_operations_get_neowall_shader_dir(void) {
    const char *home = g_get_home_dir();
    if (!home) return NULL;

    char *dir_path = g_build_filename(home, NEOWALL_SHADER_DIR, NULL);
    g_mkdir_with_parents(dir_path, 0755);
    return dir_path;
}

/* Expand ~ to home directory */
static char *expand_tilde(const char *path) {
    if (!path) return NULL;
    
    if (path[0] == '~' && (path[1] == '/' || path[1] == '\0')) {
        const char *home = g_get_home_dir();
        if (!home) return g_strdup(path);
        return g_strdup_printf("%s%s", home, path + 1);
    }
    
    return g_strdup(path);
}

/* Get neowall config path */
static char *get_neowall_config_path(void) {
    const char *home = g_get_home_dir();
    if (!home) return NULL;

    const char *xdg_config = g_getenv("XDG_CONFIG_HOME");
    if (xdg_config && *xdg_config) {
        return g_build_filename(xdg_config, "neowall", "config.vibe", NULL);
    }
    return g_build_filename(home, ".config", "neowall", "config.vibe", NULL);
}

/* Check if config has a folder (ends with /) or single file */
static bool config_has_shader_folder(char **folder_path) {
    char *config_path = get_neowall_config_path();
    if (!config_path) return false;

    char *content = NULL;
    if (!g_file_get_contents(config_path, &content, NULL, NULL)) {
        g_free(config_path);
        return false;
    }
    g_free(config_path);

    bool is_folder = false;
    char **lines = g_strsplit(content, "\n", -1);
    
    for (int i = 0; lines[i] != NULL; i++) {
        char *trimmed = g_strstrip(g_strdup(lines[i]));
        
        /* Look for "shader " but not "shader_" */
        if (g_str_has_prefix(trimmed, "shader ") && !g_str_has_prefix(trimmed, "shader_")) {
            const char *value = trimmed + 7;
            while (*value == ' ' || *value == '\t') value++;
            
            size_t len = strlen(value);
            if (len > 0 && value[len - 1] == '/') {
                is_folder = true;
                if (folder_path) {
                    char *expanded = expand_tilde(value);
                    /* Remove trailing slash */
                    size_t elen = strlen(expanded);
                    if (elen > 0 && expanded[elen - 1] == '/') {
                        expanded[elen - 1] = '\0';
                    }
                    *folder_path = expanded;
                }
            }
            g_free(trimmed);
            break;
        }
        g_free(trimmed);
    }

    g_strfreev(lines);
    g_free(content);
    return is_folder;
}

/* Update config file with shader filename, preserving show_fps */
static bool update_neowall_config(const char *shader_filename, char **error) {
    char *config_path = get_neowall_config_path();
    if (!config_path) {
        if (error) *error = g_strdup("Cannot get config path");
        return false;
    }

    char *content = NULL;
    bool has_show_fps = false;
    char *show_fps_value = NULL;

    /* Read existing config to preserve show_fps */
    if (g_file_get_contents(config_path, &content, NULL, NULL)) {
        char **lines = g_strsplit(content, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            char *trimmed = g_strstrip(g_strdup(lines[i]));
            if (g_str_has_prefix(trimmed, "show_fps")) {
                has_show_fps = true;
                const char *val = trimmed + 8;
                while (*val == ' ' || *val == '\t') val++;
                show_fps_value = g_strdup(val);
            }
            g_free(trimmed);
        }
        g_strfreev(lines);
        g_free(content);
    }

    /* Create config directory if needed */
    char *config_dir = g_path_get_dirname(config_path);
    g_mkdir_with_parents(config_dir, 0755);
    g_free(config_dir);

    /* Write new config */
    GString *new_config = g_string_new("default {\n");
    g_string_append_printf(new_config, "  shader %s\n", shader_filename);
    g_string_append(new_config, "  shader_speed 1.0\n");
    if (has_show_fps && show_fps_value) {
        g_string_append_printf(new_config, "  show_fps %s\n", show_fps_value);
        g_free(show_fps_value);
    } else {
        g_string_append(new_config, "  show_fps true\n");
    }
    g_string_append(new_config, "}\n");

    GError *gerror = NULL;
    bool success = g_file_set_contents(config_path, new_config->str, -1, &gerror);
    g_string_free(new_config, TRUE);
    g_free(config_path);

    if (!success) {
        if (error) *error = g_strdup(gerror->message);
        g_error_free(gerror);
        return false;
    }

    return true;
}

/* Find shader index from neowall list output */
static int find_shader_index(const char *shader_filename) {
    FILE *pipe = popen("neowall list 2>/dev/null", "r");
    if (!pipe) return -1;

    GString *output = g_string_new("");
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) {
        g_string_append(output, buf);
    }
    pclose(pipe);

    int found_index = -1;
    char **lines = g_strsplit(output->str, "\n", -1);
    
    for (int i = 0; lines[i] != NULL; i++) {
        char *bracket = strstr(lines[i], "[");
        if (bracket) {
            int idx = -1;
            char name[256] = {0};
            if (sscanf(bracket, "[%d] %255s", &idx, name) == 2) {
                if (strcmp(name, shader_filename) == 0) {
                    found_index = idx;
                    break;
                }
            }
        }
    }

    g_strfreev(lines);
    g_string_free(output, TRUE);
    return found_index;
}

bool file_operations_install_to_neowall(const char *shader_code,
                                        const char *shader_name,
                                        char **error) {
    if (!shader_code || !shader_name) {
        if (error) *error = g_strdup("Invalid shader code or name");
        return false;
    }

    /* Generate timestamp filename */
    GDateTime *now = g_date_time_new_now_local();
    char *timestamp_name = g_date_time_format(now, "shader_%Y%m%d_%H%M%S.glsl");
    g_date_time_unref(now);

    char *folder_path = NULL;
    bool is_folder = config_has_shader_folder(&folder_path);

    if (is_folder && folder_path) {
        /*
         * FOLDER MODE:
         * 1. Save shader with timestamp to folder
         * 2. Restart neowall
         * 3. Get list, find index
         * 4. neowall set <index>
         */
        g_mkdir_with_parents(folder_path, 0755);
        
        char *shader_path = g_build_filename(folder_path, timestamp_name, NULL);
        g_free(folder_path);

        bool success = file_operations_save_file(shader_path, shader_code, error);
        g_free(shader_path);

        if (!success) {
            g_free(timestamp_name);
            return false;
        }

        /* Restart neowall */
        system("neowall kill 2>/dev/null");
        g_usleep(100000); /* 100ms */
        system("neowall &");
        g_usleep(300000); /* 300ms for daemon to start */

        /* Find and set the new shader */
        int index = find_shader_index(timestamp_name);
        if (index >= 0) {
            char *cmd = g_strdup_printf("neowall set %d", index);
            system(cmd);
            g_free(cmd);
        }

        g_free(timestamp_name);
        return true;

    } else {
        /*
         * SINGLE FILE MODE:
         * 1. Save shader to shaders folder
         * 2. Update config to point to that file (preserve show_fps)
         * 3. Restart neowall
         */
        g_free(folder_path);

        char *shader_dir = file_operations_get_neowall_shader_dir();
        if (!shader_dir) {
            if (error) *error = g_strdup("Failed to get shader directory");
            g_free(timestamp_name);
            return false;
        }

        char *shader_path = g_build_filename(shader_dir, timestamp_name, NULL);
        g_free(shader_dir);

        bool success = file_operations_save_file(shader_path, shader_code, error);
        g_free(shader_path);

        if (!success) {
            g_free(timestamp_name);
            return false;
        }

        /* Update config */
        if (!update_neowall_config(timestamp_name, error)) {
            g_free(timestamp_name);
            return false;
        }

        g_free(timestamp_name);

        /* Restart neowall */
        system("neowall kill 2>/dev/null");
        g_usleep(100000);
        system("neowall &");

        return true;
    }
}

bool file_operations_file_exists(const char *path) {
    if (!path) return false;
    return g_file_test(path, G_FILE_TEST_EXISTS);
}

const char *file_operations_get_filename(const char *path) {
    if (!path) return NULL;
    const char *filename = strrchr(path, '/');
    if (filename) return filename + 1;
    return path;
}

const char *file_operations_get_extension(const char *path) {
    if (!path) return NULL;
    const char *ext = strrchr(path, '.');
    if (ext && ext != path) return ext + 1;
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

    if (title) gtk_window_set_title(GTK_WINDOW(dialog), title);

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

    if (title) gtk_window_set_title(GTK_WINDOW(dialog), title);
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

    if (title) gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}