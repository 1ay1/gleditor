/* Template Selection Dialog - Implementation
 * Provides a beautiful dialog for selecting shader templates with descriptions
 */

#include "editor_templates.h"
#include <string.h>

/* Template definitions */
static const TemplateInfo templates[] = {
    {
        "cosmic_tunnel",
        "🌌 Cosmic Tunnel",
        "Mesmerizing raymarched tunnel with flowing energy and vibrant colors",
        "// Cosmic Tunnel - NeoWall Shader Editor Demo\n"
        "// A mesmerizing raymarched tunnel with flowing energy\n"
        "\n"
        "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
        "    // Normalized coordinates centered at origin\n"
        "    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;\n"
        "    \n"
        "    // Create rotating tunnel effect\n"
        "    float t = iTime * 0.5;\n"
        "    float angle = atan(uv.y, uv.x);\n"
        "    float radius = length(uv);\n"
        "    \n"
        "    // Tunnel depth with perspective\n"
        "    float depth = 1.0 / (radius + 0.1);\n"
        "    \n"
        "    // Animated tunnel coordinates\n"
        "    vec2 tunnel = vec2(angle * 3.0, depth + t * 2.0);\n"
        "    \n"
        "    // Flowing energy patterns\n"
        "    float pattern = sin(tunnel.x * 4.0 + tunnel.y * 2.0) * 0.5 + 0.5;\n"
        "    pattern *= sin(tunnel.x * 2.0 - tunnel.y * 3.0 + t) * 0.5 + 0.5;\n"
        "    \n"
        "    // Circular rings\n"
        "    float rings = sin(depth * 10.0 - t * 4.0) * 0.5 + 0.5;\n"
        "    rings = pow(rings, 3.0);\n"
        "    \n"
        "    // Radial glow\n"
        "    float glow = 1.0 - smoothstep(0.0, 2.0, radius);\n"
        "    glow = pow(glow, 2.0);\n"
        "    \n"
        "    // Combine effects\n"
        "    float combined = pattern * rings + glow * 0.3;\n"
        "    \n"
        "    // Vibrant cosmic colors\n"
        "    vec3 color1 = vec3(0.5, 0.0, 1.0);  // Purple\n"
        "    vec3 color2 = vec3(0.0, 0.8, 1.0);  // Cyan\n"
        "    vec3 color3 = vec3(1.0, 0.2, 0.5);  // Pink\n"
        "    \n"
        "    // Color cycling based on position and time\n"
        "    vec3 col = mix(color1, color2, sin(tunnel.y * 0.5) * 0.5 + 0.5);\n"
        "    col = mix(col, color3, sin(tunnel.x * 0.3 + t) * 0.5 + 0.5);\n"
        "    \n"
        "    // Apply patterns and enhance brightness\n"
        "    col *= combined * 2.0;\n"
        "    \n"
        "    // Add sparkles\n"
        "    float sparkle = sin(tunnel.x * 20.0) * sin(tunnel.y * 15.0);\n"
        "    sparkle = pow(max(0.0, sparkle), 10.0);\n"
        "    col += vec3(sparkle) * 2.0;\n"
        "    \n"
        "    // Vignette effect\n"
        "    col *= 1.0 - radius * 0.3;\n"
        "    \n"
        "    // Output with gamma correction\n"
        "    fragColor = vec4(pow(col, vec3(0.8)), 1.0);\n"
        "}\n"
    },
    {
        "plasma",
        "🌊 Plasma Wave",
        "Colorful plasma effect with smooth flowing waves",
        "// Plasma Wave Effect\n"
        "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
        "    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;\n"
        "    \n"
        "    float d1 = length(uv - vec2(sin(iTime * 0.3), cos(iTime * 0.5)));\n"
        "    float d2 = length(uv - vec2(cos(iTime * 0.4), sin(iTime * 0.6)));\n"
        "    \n"
        "    float plasma = sin(d1 * 10.0 + iTime) + cos(d2 * 8.0 - iTime);\n"
        "    vec3 col = 0.5 + 0.5 * cos(plasma + vec3(0, 2, 4));\n"
        "    \n"
        "    fragColor = vec4(col, 1.0);\n"
        "}\n"
    },
    {
        "raymarching",
        "🎯 Raymarching Sphere",
        "3D raymarched sphere with lighting and shadows",
        "// Raymarching Sphere\n"
        "float sdSphere(vec3 p, float r) {\n"
        "    return length(p) - r;\n"
        "}\n"
        "\n"
        "float map(vec3 p) {\n"
        "    return sdSphere(p - vec3(0, 0, 3), 1.0);\n"
        "}\n"
        "\n"
        "vec3 getNormal(vec3 p) {\n"
        "    vec2 e = vec2(0.001, 0.0);\n"
        "    return normalize(vec3(\n"
        "        map(p + e.xyy) - map(p - e.xyy),\n"
        "        map(p + e.yxy) - map(p - e.yxy),\n"
        "        map(p + e.yyx) - map(p - e.yyx)\n"
        "    ));\n"
        "}\n"
        "\n"
        "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
        "    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;\n"
        "    \n"
        "    vec3 ro = vec3(0, 0, 0);\n"
        "    vec3 rd = normalize(vec3(uv, 1.0));\n"
        "    \n"
        "    float t = 0.0;\n"
        "    for (int i = 0; i < 64; i++) {\n"
        "        vec3 p = ro + rd * t;\n"
        "        float d = map(p);\n"
        "        if (d < 0.001) break;\n"
        "        t += d;\n"
        "        if (t > 20.0) break;\n"
        "    }\n"
        "    \n"
        "    vec3 col = vec3(0.1, 0.1, 0.2);\n"
        "    if (t < 20.0) {\n"
        "        vec3 p = ro + rd * t;\n"
        "        vec3 n = getNormal(p);\n"
        "        vec3 light = normalize(vec3(1, 1, -1));\n"
        "        float diff = max(dot(n, light), 0.0);\n"
        "        col = vec3(0.5, 0.7, 1.0) * diff + vec3(0.1, 0.1, 0.2);\n"
        "    }\n"
        "    \n"
        "    fragColor = vec4(col, 1.0);\n"
        "}\n"
    },
    {
        "fractal",
        "🌀 Mandelbrot Fractal",
        "Beautiful animated Mandelbrot set fractal zoom",
        "// Mandelbrot Fractal\n"
        "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
        "    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;\n"
        "    \n"
        "    // Zoom and pan\n"
        "    float zoom = 2.0 + sin(iTime * 0.2) * 0.5;\n"
        "    vec2 c = uv * zoom + vec2(-0.5, 0.0);\n"
        "    \n"
        "    vec2 z = vec2(0.0);\n"
        "    float iter = 0.0;\n"
        "    const int maxIter = 100;\n"
        "    \n"
        "    for (int i = 0; i < maxIter; i++) {\n"
        "        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;\n"
        "        if (length(z) > 2.0) break;\n"
        "        iter++;\n"
        "    }\n"
        "    \n"
        "    float t = iter / float(maxIter);\n"
        "    vec3 col = 0.5 + 0.5 * cos(t * 6.28 + vec3(0, 2, 4) + iTime);\n"
        "    \n"
        "    fragColor = vec4(col, 1.0);\n"
        "}\n"
    },
    {
        "water",
        "💧 Water Ripples",
        "Realistic water ripple effect with reflections",
        "// Water Ripples\n"
        "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
        "    vec2 uv = fragCoord / iResolution.xy;\n"
        "    vec2 p = uv * 2.0 - 1.0;\n"
        "    p.x *= iResolution.x / iResolution.y;\n"
        "    \n"
        "    // Create ripples\n"
        "    float d = length(p);\n"
        "    float ripple = sin(d * 10.0 - iTime * 3.0) * 0.5 + 0.5;\n"
        "    ripple *= exp(-d * 2.0);\n"
        "    \n"
        "    // Multiple ripple sources\n"
        "    vec2 p1 = p - vec2(sin(iTime), cos(iTime)) * 0.3;\n"
        "    float d1 = length(p1);\n"
        "    ripple += sin(d1 * 15.0 - iTime * 4.0) * exp(-d1 * 3.0) * 0.3;\n"
        "    \n"
        "    // Water colors\n"
        "    vec3 col = vec3(0.1, 0.3, 0.5) + ripple * vec3(0.2, 0.4, 0.6);\n"
        "    \n"
        "    // Add highlights\n"
        "    col += pow(ripple, 8.0) * vec3(1.0, 1.0, 0.8);\n"
        "    \n"
        "    fragColor = vec4(col, 1.0);\n"
        "}\n"
    },
    {
        "starfield",
        "⭐ Starfield",
        "Flying through a field of stars at warp speed",
        "// Starfield\n"
        "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
        "    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;\n"
        "    vec3 col = vec3(0.0);\n"
        "    \n"
        "    // Create layers of stars\n"
        "    for (float i = 0.0; i < 3.0; i++) {\n"
        "        vec2 p = uv * (3.0 + i);\n"
        "        p.y += iTime * (0.5 + i * 0.3);\n"
        "        \n"
        "        vec2 id = floor(p);\n"
        "        vec2 gv = fract(p) - 0.5;\n"
        "        \n"
        "        float n = fract(sin(dot(id, vec2(12.9898, 78.233))) * 43758.5453);\n"
        "        float size = 0.05 * (1.0 - i * 0.3);\n"
        "        \n"
        "        float d = length(gv);\n"
        "        float star = smoothstep(size, size * 0.1, d);\n"
        "        star *= n;\n"
        "        \n"
        "        col += star * vec3(0.8 + i * 0.1, 0.8, 1.0) * (1.0 + i);\n"
        "    }\n"
        "    \n"
        "    fragColor = vec4(col, 1.0);\n"
        "}\n"
    },
    {
        "gradient",
        "🎨 Simple Gradient",
        "Basic animated color gradient - perfect for beginners",
        "// Simple Gradient\n"
        "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
        "    vec2 uv = fragCoord / iResolution.xy;\n"
        "    \n"
        "    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));\n"
        "    \n"
        "    fragColor = vec4(col, 1.0);\n"
        "}\n"
    },
    {
        "blank",
        "📄 Blank Template",
        "Empty Shadertoy-compatible template to start from scratch",
        "// Blank Shader Template\n"
        "// Available uniforms:\n"
        "//   iTime       - shader playback time (seconds)\n"
        "//   iResolution - viewport resolution (pixels)\n"
        "//   iMouse      - mouse pixel coordinates\n"
        "\n"
        "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
        "    vec2 uv = fragCoord / iResolution.xy;\n"
        "    \n"
        "    // Your code here\n"
        "    \n"
        "    fragColor = vec4(uv, 0.5, 1.0);\n"
        "}\n"
    }
};

static const size_t template_count = sizeof(templates) / sizeof(templates[0]);

/* Dialog state */
static struct {
    GtkWidget *dialog;
    GtkWidget *list_box;
    char *selected_code;
} dialog_state = {
    .dialog = NULL,
    .list_box = NULL,
    .selected_code = NULL
};

/* List box row selection callback */
static void on_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)box;
    (void)user_data;

    if (!row) return;

    int index = gtk_list_box_row_get_index(row);
    if (index >= 0 && index < (int)template_count) {
        if (dialog_state.selected_code) {
            g_free(dialog_state.selected_code);
        }
        dialog_state.selected_code = g_strdup(templates[index].code);
    }
}

/* List box row activation (double-click) callback */
static void on_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)box;
    (void)user_data;

    on_row_selected(NULL, row, NULL);

    if (dialog_state.dialog) {
        gtk_dialog_response(GTK_DIALOG(dialog_state.dialog), GTK_RESPONSE_ACCEPT);
    }
}

/* Create a template list item */
static GtkWidget *create_template_item(const TemplateInfo *info) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);

    /* Title with emoji */
    GtkWidget *title = gtk_label_new(NULL);
    char *markup = g_markup_printf_escaped("<span size='large' weight='bold'>%s</span>",
                                           info->display_name);
    gtk_label_set_markup(GTK_LABEL(title), markup);
    g_free(markup);
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

    /* Description */
    GtkWidget *desc = gtk_label_new(info->description);
    gtk_label_set_xalign(GTK_LABEL(desc), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(desc), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(desc), 50);
    gtk_widget_set_opacity(desc, 0.7);
    gtk_box_pack_start(GTK_BOX(box), desc, FALSE, FALSE, 0);

    gtk_widget_show_all(box);
    return box;
}

/* Public API Implementation */

char *editor_templates_show_dialog(GtkWindow *parent) {
    /* Create dialog */
    dialog_state.dialog = gtk_dialog_new_with_buttons(
        "New Shader from Template",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Create", GTK_RESPONSE_ACCEPT,
        NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog_state.dialog), 500, 600);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog_state.dialog));

    /* Add header label */
    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header),
                         "<big><b>Choose a template to get started</b></big>");
    gtk_widget_set_margin_start(header, 12);
    gtk_widget_set_margin_end(header, 12);
    gtk_widget_set_margin_top(header, 12);
    gtk_widget_set_margin_bottom(header, 12);
    gtk_box_pack_start(GTK_BOX(content), header, FALSE, FALSE, 0);

    /* Create scrolled window */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(content), scrolled, TRUE, TRUE, 0);

    /* Create list box */
    dialog_state.list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(dialog_state.list_box),
                                     GTK_SELECTION_SINGLE);
    g_signal_connect(dialog_state.list_box, "row-selected",
                     G_CALLBACK(on_row_selected), NULL);
    g_signal_connect(dialog_state.list_box, "row-activated",
                     G_CALLBACK(on_row_activated), NULL);

    gtk_container_add(GTK_CONTAINER(scrolled), dialog_state.list_box);

    /* Add templates to list */
    for (size_t i = 0; i < template_count; i++) {
        GtkWidget *item = create_template_item(&templates[i]);
        gtk_list_box_insert(GTK_LIST_BOX(dialog_state.list_box), item, -1);
    }

    /* Select first item by default */
    GtkListBoxRow *first_row = gtk_list_box_get_row_at_index(
        GTK_LIST_BOX(dialog_state.list_box), 0);
    if (first_row) {
        gtk_list_box_select_row(GTK_LIST_BOX(dialog_state.list_box), first_row);
        on_row_selected(NULL, first_row, NULL);
    }

    gtk_widget_show_all(dialog_state.dialog);

    /* Run dialog */
    int response = gtk_dialog_run(GTK_DIALOG(dialog_state.dialog));

    char *result = NULL;
    if (response == GTK_RESPONSE_ACCEPT && dialog_state.selected_code) {
        result = g_strdup(dialog_state.selected_code);
    }

    /* Cleanup */
    if (dialog_state.selected_code) {
        g_free(dialog_state.selected_code);
        dialog_state.selected_code = NULL;
    }

    gtk_widget_destroy(dialog_state.dialog);
    dialog_state.dialog = NULL;
    dialog_state.list_box = NULL;

    return result;
}

const TemplateInfo *editor_templates_get_list(size_t *count) {
    if (count) {
        *count = template_count;
    }
    return templates;
}

const char *editor_templates_get_code(const char *name) {
    if (!name) return NULL;

    for (size_t i = 0; i < template_count; i++) {
        if (strcmp(templates[i].name, name) == 0) {
            return templates[i].code;
        }
    }

    return NULL;
}
