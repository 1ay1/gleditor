/* NeoWall Shader Editor - Standalone Application
 * Live GLSL shader editor with real-time preview
 *
 * Features:
 * - Real-time shader compilation and preview
 * - Syntax highlighting for GLSL
 * - OpenGL ES 2.0/3.0+ support
 * - Shadertoy compatibility layer
 * - Multiple shader templates
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "shader_editor.h"

#define APP_ID "com.neowall.gleditor"
#define APP_NAME "NeoWall Shader Editor"

/* Application state */
typedef struct {
    GtkApplication *app;
    bool show_version;
    bool show_help;
} AppContext;

static AppContext app_context = {0};

/* Command line options */
static gboolean opt_version = FALSE;
static gboolean opt_verbose = FALSE;

static GOptionEntry options[] = {
    { "version", 'v', 0, G_OPTION_ARG_NONE, &opt_version,
      "Show version information", NULL },
    { "verbose", 'V', 0, G_OPTION_ARG_NONE, &opt_verbose,
      "Enable verbose output", NULL },
    { NULL }
};

/* Print version information */
static void print_version(void) {
    printf("%s version %s\n", APP_NAME, VERSION);
    printf("OpenGL ES Shader Editor for NeoWall\n\n");

    printf("Supported OpenGL ES versions:\n");
#ifdef HAVE_GLES1
    printf("  ✓ OpenGL ES 1.x (legacy)\n");
#endif
#ifdef HAVE_GLES2
    printf("  ✓ OpenGL ES 2.0\n");
#endif
#ifdef HAVE_GLES30
    printf("  ✓ OpenGL ES 3.0\n");
#endif
#ifdef HAVE_GLES31
    printf("  ✓ OpenGL ES 3.1\n");
#endif
#ifdef HAVE_GLES32
    printf("  ✓ OpenGL ES 3.2\n");
#endif
    printf("\n");
}

/* Print help information */
static void print_help(void) {
    printf("Usage: gleditor [OPTIONS]\n\n");
    printf("NeoWall Shader Editor - Live GLSL shader editor with preview\n\n");
    printf("Options:\n");
    printf("  -v, --version     Show version information\n");
    printf("  -V, --verbose     Enable verbose output\n");
    printf("  -h, --help        Show this help message\n");
    printf("\n");
    printf("Features:\n");
    printf("  • Real-time shader compilation and preview\n");
    printf("  • GLSL syntax highlighting\n");
    printf("  • Shadertoy compatibility\n");
    printf("  • Multiple shader templates\n");
    printf("  • Error reporting with line numbers\n");
    printf("  • Save/Load shader files\n");
    printf("\n");
    printf("Keyboard Shortcuts:\n");
    printf("  Ctrl+S            Save current shader\n");
    printf("  Ctrl+O            Load shader from file\n");
    printf("  Ctrl+N            New shader (reset to template)\n");
    printf("  Ctrl+R            Recompile shader\n");
    printf("  Ctrl+Q            Quit application\n");
    printf("  F11               Toggle fullscreen preview\n");
    printf("  Space             Pause/Resume animation\n");
    printf("\n");
}

/* Application startup */
static void on_app_startup(GtkApplication *app, gpointer user_data) {
    (void)app;
    (void)user_data;

    if (opt_verbose) {
        printf("Application starting...\n");
        printf("GTK version: %d.%d.%d\n",
               gtk_get_major_version(),
               gtk_get_minor_version(),
               gtk_get_micro_version());
    }
}

/* Application activation */
static void on_app_activate(GtkApplication *app, gpointer user_data) {
    (void)app;
    (void)user_data;

    if (opt_verbose) {
        printf("Activating shader editor window...\n");
    }

    /* Show the shader editor */
    shader_editor_show(app);

    if (opt_verbose) {
        printf("Shader editor window opened\n");
    }
}

/* Application shutdown */
static void on_app_shutdown(GtkApplication *app, gpointer user_data) {
    (void)app;
    (void)user_data;

    if (opt_verbose) {
        printf("Application shutting down...\n");
    }

    /* Close shader editor if still open */
    if (shader_editor_is_open()) {
        shader_editor_close();
    }
}

/* Handle command line arguments */
static gint on_command_line(GtkApplication *app,
                           GApplicationCommandLine *cmdline,
                           gpointer user_data) {
    (void)user_data;

    gint argc;
    gchar **argv = g_application_command_line_get_arguments(cmdline, &argc);

    GOptionContext *context = g_option_context_new("- Shader Editor");
    g_option_context_add_main_entries(context, options, NULL);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));

    GError *error = NULL;
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_application_command_line_printerr(cmdline,
                                           "Error parsing options: %s\n",
                                           error->message);
        g_error_free(error);
        g_option_context_free(context);
        g_strfreev(argv);
        return 1;
    }

    g_option_context_free(context);

    /* Handle version flag */
    if (opt_version) {
        print_version();
        g_strfreev(argv);
        return 0;
    }

    /* Activate the application (show window) */
    g_application_activate(G_APPLICATION(app));

    g_strfreev(argv);
    return 0;
}

/* Main entry point */
int main(int argc, char *argv[]) {
    int status;

    /* Handle help flag early (before GTK initialization) */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
    }

    /* Create GTK application */
    GtkApplication *app = gtk_application_new(APP_ID,
                                             G_APPLICATION_HANDLES_COMMAND_LINE);

    if (app == NULL) {
        fprintf(stderr, "Error: Failed to create GTK application\n");
        return 1;
    }

    app_context.app = app;

    /* Connect signals */
    g_signal_connect(app, "startup", G_CALLBACK(on_app_startup), &app_context);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), &app_context);
    g_signal_connect(app, "shutdown", G_CALLBACK(on_app_shutdown), &app_context);
    g_signal_connect(app, "command-line", G_CALLBACK(on_command_line), &app_context);

    /* Run application */
    status = g_application_run(G_APPLICATION(app), argc, argv);

    /* Cleanup */
    g_object_unref(app);

    return status;
}
