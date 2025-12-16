/* Help Dialog Component - Implementation
 * Displays keyboard shortcuts, tips, and helpful information
 */

#include "editor_help.h"
#include <string.h>

/* Suppress deprecated function warnings (gtk_widget_override_font) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/* Help dialog content structure */
typedef struct {
    const char *category;
    const char *action;
    const char *shortcut;
} HelpItem;

/* Keyboard shortcuts and help items */
static const HelpItem help_items[] = {
    /* File Operations */
    {"📁 File", "New File", "Ctrl+N"},
    {"📁 File", "Open File", "Ctrl+O"},
    {"📁 File", "Save File", "Ctrl+S"},
    {"📁 File", "Save As", "Ctrl+Shift+S"},
    {"📁 File", "Close", "Ctrl+W"},

    /* Editing */
    {"✏️ Editing", "Undo", "Ctrl+Z"},
    {"✏️ Editing", "Redo", "Ctrl+Y / Ctrl+Shift+Z"},
    {"✏️ Editing", "Cut", "Ctrl+X"},
    {"✏️ Editing", "Copy", "Ctrl+C"},
    {"✏️ Editing", "Paste", "Ctrl+V"},
    {"✏️ Editing", "Select All", "Ctrl+A"},
    {"✏️ Editing", "Find", "Ctrl+F"},
    {"✏️ Editing", "Replace", "Ctrl+H"},
    {"✏️ Editing", "Comment/Uncomment", "Ctrl+/"},

    /* Code Assistance */
    {"🔧 Code", "Auto-Complete", "Ctrl+Space"},
    {"🔧 Code", "Indent Selection", "Tab"},
    {"🔧 Code", "Unindent Selection", "Shift+Tab"},
    {"🔧 Code", "Duplicate Line", "Ctrl+D"},
    {"🔧 Code", "Delete Line", "Ctrl+Shift+K"},
    {"🔧 Code", "Move Line Up", "Alt+Up"},
    {"🔧 Code", "Move Line Down", "Alt+Down"},

    /* Compilation & Preview */
    {"🎨 Shader", "Compile Shader", "F5 / Ctrl+B"},
    {"🎨 Shader", "Toggle Auto-Compile", "Ctrl+Shift+A"},
    {"🎨 Shader", "Show Error Panel", "Ctrl+E"},

    /* View */
    {"👁️ View", "Toggle Split Orientation", "F6"},
    {"👁️ View", "Show Only Editor", "F7"},
    {"👁️ View", "Show Only Preview", "F8"},
    {"👁️ View", "Show Both Panels", "F9"},
    {"👁️ View", "Settings Dialog", "Ctrl+,"},
    {"👁️ View", "Help Dialog", "F1"},

    /* Navigation */
    {"🧭 Navigate", "Go to Line", "Ctrl+G"},
    {"🧭 Navigate", "Jump to Start", "Ctrl+Home"},
    {"🧭 Navigate", "Jump to End", "Ctrl+End"},
    {"🧭 Navigate", "Previous Word", "Ctrl+Left"},
    {"🧭 Navigate", "Next Word", "Ctrl+Right"},

    {NULL, NULL, NULL}
};

/* Tips and tricks */
static const char *tips[] = {
    "💡 Use Ctrl+Space to trigger GLSL auto-completion with 170+ keywords and functions",
    "💡 Type 'mainImage' and press Tab to insert a Shadertoy template",
    "💡 Enable Auto-Compile in settings for instant shader preview as you type",
    "💡 The editor supports all Shadertoy uniforms: iTime, iResolution, iMouse, etc.",
    "💡 Use F6 to toggle between horizontal and vertical split layouts",
    "💡 Background Pattern setting shows a subtle grid to help with code alignment",
    "💡 Smart Home/End makes the Home key jump to first non-whitespace character",
    "💡 Click the error status in the status bar to show the full compilation log",
    "💡 Settings are saved automatically to ~/.config/gleditor/settings.conf",
    "💡 Try different color themes in Settings → Appearance → Theme",
    NULL
};

/* GLSL quick reference */
static const char *glsl_reference[] = {
    "Common GLSL Types:",
    "  float, vec2, vec3, vec4",
    "  int, ivec2, ivec3, ivec4",
    "  mat2, mat3, mat4",
    "  sampler2D, samplerCube",
    "",
    "Common Functions:",
    "  mix(a, b, t) - Linear interpolation",
    "  smoothstep(e0, e1, x) - Smooth interpolation",
    "  clamp(x, min, max) - Constrain value",
    "  length(v) - Vector magnitude",
    "  normalize(v) - Unit vector",
    "  dot(a, b) - Dot product",
    "  cross(a, b) - Cross product",
    "  sin/cos/tan/atan - Trigonometry",
    "  texture(sampler, uv) - Sample texture",
    "",
    "Shadertoy Uniforms:",
    "  iTime - Elapsed time in seconds",
    "  iResolution - Viewport resolution",
    "  iMouse - Mouse position/state",
    "  iFrame - Current frame number",
    "  iChannel0-3 - Input textures",
    NULL
};

/* Create help content as a scrolled text view */
static GtkWidget* create_help_content(void) {
    GtkWidget *notebook = gtk_notebook_new();

    /* ===== SHORTCUTS TAB ===== */
    GtkWidget *shortcuts_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(shortcuts_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *shortcuts_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(shortcuts_grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(shortcuts_grid), 24);
    gtk_container_set_border_width(GTK_CONTAINER(shortcuts_grid), 16);

    /* Group shortcuts by category */
    int row = 0;
    const char *last_category = NULL;

    for (int i = 0; help_items[i].action != NULL; i++) {
        /* Add category header if it changed */
        if (last_category == NULL || strcmp(last_category, help_items[i].category) != 0) {
            if (last_category != NULL) {
                /* Add spacing between categories */
                row++;
            }

            GtkWidget *category_label = gtk_label_new(NULL);
            char markup[256];
            snprintf(markup, sizeof(markup), "<b>%s</b>", help_items[i].category);
            gtk_label_set_markup(GTK_LABEL(category_label), markup);
            gtk_widget_set_halign(category_label, GTK_ALIGN_START);
            gtk_grid_attach(GTK_GRID(shortcuts_grid), category_label, 0, row, 2, 1);
            row++;

            last_category = help_items[i].category;
        }

        /* Action label */
        GtkWidget *action_label = gtk_label_new(help_items[i].action);
        gtk_widget_set_halign(action_label, GTK_ALIGN_START);
        gtk_widget_set_margin_start(action_label, 20);
        gtk_grid_attach(GTK_GRID(shortcuts_grid), action_label, 0, row, 1, 1);

        /* Shortcut label (styled as keyboard key) */
        GtkWidget *shortcut_label = gtk_label_new(NULL);
        char markup[256];
        snprintf(markup, sizeof(markup), "<tt><b>%s</b></tt>", help_items[i].shortcut);
        gtk_label_set_markup(GTK_LABEL(shortcut_label), markup);
        gtk_widget_set_halign(shortcut_label, GTK_ALIGN_END);

        /* Add visual styling to shortcut */
        GtkWidget *shortcut_box = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(shortcut_box), shortcut_label);
        gtk_widget_set_name(shortcut_box, "shortcut-key");

        gtk_grid_attach(GTK_GRID(shortcuts_grid), shortcut_box, 1, row, 1, 1);
        row++;
    }

    gtk_container_add(GTK_CONTAINER(shortcuts_scroll), shortcuts_grid);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), shortcuts_scroll,
                            gtk_label_new("⌨️ Shortcuts"));

    /* ===== TIPS TAB ===== */
    GtkWidget *tips_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tips_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *tips_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(tips_box), 16);

    GtkWidget *tips_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tips_title),
                        "<span size='large'><b>💡 Tips &amp; Tricks</b></span>");
    gtk_widget_set_halign(tips_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(tips_box), tips_title, FALSE, FALSE, 8);

    for (int i = 0; tips[i] != NULL; i++) {
        GtkWidget *tip_label = gtk_label_new(tips[i]);
        gtk_label_set_line_wrap(GTK_LABEL(tip_label), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(tip_label), 80);
        gtk_widget_set_halign(tip_label, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(tips_box), tip_label, FALSE, FALSE, 0);
    }

    gtk_container_add(GTK_CONTAINER(tips_scroll), tips_box);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tips_scroll,
                            gtk_label_new("💡 Tips"));

    /* ===== GLSL REFERENCE TAB ===== */
    GtkWidget *glsl_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(glsl_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *glsl_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(glsl_text), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(glsl_text), FALSE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(glsl_text), 16);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(glsl_text), 16);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(glsl_text), 16);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(glsl_text), 16);

    /* Set monospace font for code reference */
    PangoFontDescription *font = pango_font_description_from_string("Monospace 10");
    gtk_widget_override_font(glsl_text, font);
    pango_font_description_free(font);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(glsl_text));
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(buffer, &iter);

    for (int i = 0; glsl_reference[i] != NULL; i++) {
        gtk_text_buffer_insert(buffer, &iter, glsl_reference[i], -1);
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    }

    gtk_container_add(GTK_CONTAINER(glsl_scroll), glsl_text);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), glsl_scroll,
                            gtk_label_new("📖 GLSL Ref"));

    /* ===== ABOUT TAB ===== */
    GtkWidget *about_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(about_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *about_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(about_box), 32);
    gtk_widget_set_halign(about_box, GTK_ALIGN_CENTER);

    /* Logo - Try to load SVG from multiple locations, fallback to text */
    GtkWidget *logo = NULL;
    GError *error = NULL;
    GdkPixbuf *pixbuf = NULL;

    /* Try development path first */
    pixbuf = gdk_pixbuf_new_from_file_at_scale("data/gleditor.svg", 400, 133, TRUE, &error);

    /* If not found and we have an install path, try that */
#ifdef GLEDITOR_DATADIR
    if (!pixbuf && error) {
        g_error_free(error);
        error = NULL;
        char installed_path[512];
        snprintf(installed_path, sizeof(installed_path), "%s/gleditor.svg", GLEDITOR_DATADIR);
        pixbuf = gdk_pixbuf_new_from_file_at_scale(installed_path, 400, 133, TRUE, &error);
    }
#endif

    if (pixbuf) {
        logo = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
    } else {
        if (error) g_error_free(error);
        /* Fallback to styled text logo */
        logo = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(logo),
                            "<span size='xx-large' font_family='monospace' foreground='#00ff88'><b>gleditor</b></span>");
    }
    gtk_box_pack_start(GTK_BOX(about_box), logo, FALSE, FALSE, 8);

    GtkWidget *app_subtitle = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(app_subtitle),
                        "<span size='large' style='italic'>GLSL Shader Editor for NeoWall</span>");
    gtk_box_pack_start(GTK_BOX(about_box), app_subtitle, FALSE, FALSE, 0);

    GtkWidget *version_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(version_label),
                        "<span foreground='#888'>Version 1.0.0 • MIT License</span>");
    gtk_box_pack_start(GTK_BOX(about_box), version_label, FALSE, FALSE, 16);

    /* Separator */
    GtkWidget *separator1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(about_box), separator1, FALSE, FALSE, 8);

    /* Features section */
    GtkWidget *features_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(features_title), "<b>✨ Features</b>");
    gtk_widget_set_halign(features_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(about_box), features_title, FALSE, FALSE, 4);

    const char *features[] = {
        "• Syntax highlighting for GLSL shaders",
        "• Real-time shader preview with Shadertoy compatibility",
        "• 170+ auto-completion items (keywords, functions, snippets)",
        "• Multiple color themes and customizable fonts",
        "• Configurable editor behavior and appearance",
        "• Error panel with compilation diagnostics",
        "• 20+ keyboard shortcuts for productivity",
        NULL
    };

    for (int i = 0; features[i] != NULL; i++) {
        GtkWidget *label = gtk_label_new(features[i]);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_widget_set_margin_start(label, 16);
        gtk_box_pack_start(GTK_BOX(about_box), label, FALSE, FALSE, 2);
    }

    gtk_box_pack_start(GTK_BOX(about_box), gtk_label_new(""), FALSE, FALSE, 4);

    /* Built with section */
    GtkWidget *tech_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tech_title), "<b>🔧 Built With</b>");
    gtk_widget_set_halign(tech_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(about_box), tech_title, FALSE, FALSE, 4);

    const char *tech_stack[] = {
        "• GTK+ 3",
        "• GtkSourceView 4",
        "• OpenGL ES 2.0/3.0+",
        "• EGL",
        NULL
    };

    for (int i = 0; tech_stack[i] != NULL; i++) {
        GtkWidget *label = gtk_label_new(tech_stack[i]);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_widget_set_margin_start(label, 16);
        gtk_box_pack_start(GTK_BOX(about_box), label, FALSE, FALSE, 2);
    }

    gtk_box_pack_start(GTK_BOX(about_box), gtk_label_new(""), FALSE, FALSE, 4);

    /* Links section */
    GtkWidget *links_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(links_title), "<b>🔗 Links</b>");
    gtk_widget_set_halign(links_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(about_box), links_title, FALSE, FALSE, 4);

    /* GitHub Repository link */
    GtkWidget *repo_link = gtk_link_button_new_with_label(
        "https://github.com/1ay1/gleditor",
        "📦 GitHub Repository");
    gtk_button_set_relief(GTK_BUTTON(repo_link), GTK_RELIEF_NONE);
    gtk_widget_set_halign(repo_link, GTK_ALIGN_START);
    gtk_widget_set_margin_start(repo_link, 16);
    gtk_box_pack_start(GTK_BOX(about_box), repo_link, FALSE, FALSE, 2);

    /* Developer link */
    GtkWidget *dev_link = gtk_link_button_new_with_label(
        "https://github.com/1ay1/",
        "👨‍💻 Developer: @1ay1");
    gtk_button_set_relief(GTK_BUTTON(dev_link), GTK_RELIEF_NONE);
    gtk_widget_set_halign(dev_link, GTK_ALIGN_START);
    gtk_widget_set_margin_start(dev_link, 16);
    gtk_box_pack_start(GTK_BOX(about_box), dev_link, FALSE, FALSE, 2);

    gtk_box_pack_start(GTK_BOX(about_box), gtk_label_new(""), FALSE, FALSE, 8);

    /* Footer */
    GtkWidget *footer = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(footer),
                        "<span size='small' foreground='#666'>Made with ❤️ for shader artists and developers</span>");
    gtk_box_pack_start(GTK_BOX(about_box), footer, FALSE, FALSE, 8);

    gtk_container_add(GTK_CONTAINER(about_scroll), about_box);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), about_scroll,
                            gtk_label_new("ℹ️ About"));

    return notebook;
}

/* Public API */
void editor_help_show_dialog(GtkWindow *parent) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "❓ gleditor Help",
        parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 600);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *help_content = create_help_content();
    gtk_box_pack_start(GTK_BOX(content), help_content, TRUE, TRUE, 0);

    /* Add some CSS for shortcut keys styling */
    GtkCssProvider *css_provider = gtk_css_provider_new();
    const char *css_data =
        "#shortcut-key {"
        "  padding: 4px 8px;"
        "  border-radius: 4px;"
        "  background: alpha(currentColor, 0.1);"
        "  border: 1px solid alpha(currentColor, 0.2);"
        "}";

    gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

#pragma GCC diagnostic pop
