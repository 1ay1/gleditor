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



/* Update the shader line in neowall config file */
static bool update_neowall_config_shader(const char *shader_filename, char **error) {
    const char *home = g_get_home_dir();
    if (!home) {
        if (error) *error = g_strdup("Cannot get home directory");
        return false;
    }

    /* Try XDG config first, then fallback */
    const char *xdg_config = g_getenv("XDG_CONFIG_HOME");
    char *config_path = NULL;
    
    if (xdg_config && *xdg_config) {
        config_path = g_build_filename(xdg_config, "neowall", "config.vibe", NULL);
    } else {
        config_path = g_build_filename(home, ".config", "neowall", "config.vibe", NULL);
    }

    /* Read existing config */
    char *config_content = NULL;
    GError *gerror = NULL;
    if (!g_file_get_contents(config_path, &config_content, NULL, &gerror)) {
        /* Config doesn't exist - create a minimal one */
        g_clear_error(&gerror);
        char *config_dir = g_path_get_dirname(config_path);
        g_mkdir_with_parents(config_dir, 0755);
        g_free(config_dir);

        char *new_config = g_strdup_printf(
            "# NeoWall Configuration\n"
            "# Auto-generated by GLEditor\n\n"
            "default {\n"
            "  shader %s\n"
            "  shader_fps 60\n"
            "}\n",
            shader_filename
        );
        
        bool success = g_file_set_contents(config_path, new_config, -1, &gerror);
        g_free(new_config);
        g_free(config_path);
        
        if (!success) {
            if (error) *error = g_strdup(gerror->message);
            g_error_free(gerror);
            return false;
        }
        return true;
    }

    /* Find and replace shader line in default block */
    GString *new_config = g_string_new("");
    char **lines = g_strsplit(config_content, "\n", -1);
    bool in_default_block = false;
    bool found_shader_line = false;
    int brace_depth = 0;

    for (int i = 0; lines[i] != NULL; i++) {
        const char *line = lines[i];
        char *trimmed = g_strstrip(g_strdup(line));

        /* Track if we're in the default block */
        if (g_str_has_prefix(trimmed, "default") && strstr(trimmed, "{")) {
            in_default_block = true;
            brace_depth = 1;
            g_string_append_printf(new_config, "%s\n", line);
            g_free(trimmed);
            continue;
        }

        if (in_default_block) {
            /* Count braces */
            for (const char *c = line; *c; c++) {
                if (*c == '{') brace_depth++;
                else if (*c == '}') brace_depth--;
            }

            /* Replace shader line */
            if (g_str_has_prefix(trimmed, "shader ") && !g_str_has_prefix(trimmed, "shader_")) {
                /* Get the indentation from original line */
                int indent = 0;
                while (line[indent] == ' ' || line[indent] == '\t') indent++;
                char *indent_str = g_strndup(line, indent);
                g_string_append_printf(new_config, "%sshader %s\n", indent_str, shader_filename);
                g_free(indent_str);
                found_shader_line = true;
            } else {
                g_string_append_printf(new_config, "%s\n", line);
            }

            if (brace_depth == 0) {
                in_default_block = false;
            }
        } else {
            g_string_append_printf(new_config, "%s\n", line);
        }

        g_free(trimmed);
    }

    g_strfreev(lines);
    g_free(config_content);

    /* If we didn't find a shader line, insert one in default block */
    if (!found_shader_line) {
        /* Fallback: create simple config */
        g_string_free(new_config, TRUE);
        char *simple_config = g_strdup_printf(
            "# NeoWall Configuration\n"
            "# Auto-generated by GLEditor\n\n"
            "default {\n"
            "  shader %s\n"
            "  shader_fps 60\n"
            "}\n",
            shader_filename
        );
        bool success = g_file_set_contents(config_path, simple_config, -1, &gerror);
        g_free(simple_config);
        g_free(config_path);
        if (!success) {
            if (error) *error = g_strdup(gerror->message);
            g_error_free(gerror);
            return false;
        }
        return true;
    }

    /* Remove trailing newline if double */
    char *final_config = new_config->str;
    size_t len = new_config->len;
    while (len > 1 && final_config[len-1] == '\n' && final_config[len-2] == '\n') {
        len--;
    }
    final_config[len] = '\0';

    bool success = g_file_set_contents(config_path, final_config, -1, &gerror);
    g_string_free(new_config, TRUE);
    g_free(config_path);

    if (!success) {
        if (error) *error = g_strdup(gerror->message);
        g_error_free(gerror);
        return false;
    }

    return true;
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

    /* Save shader to NeoWall shader directory */
    bool success = file_operations_save_file(shader_path, shader_code, error);
    g_free(shader_path);

    if (!success) {
        g_free(shader_filename);
        return false;
    }

    /* Kill any running neowall daemon */
    system("neowall kill 2>/dev/null");
    g_usleep(200000); /* 200ms for daemon to die */

    /* Update config to use this shader */
    if (!update_neowall_config_shader(shader_filename, error)) {
        g_free(shader_filename);
        return false;
    }

    g_free(shader_filename);

    /* Start neowall daemon with new config */
    system("neowall &");

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
