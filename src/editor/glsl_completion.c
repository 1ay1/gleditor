#include "glsl_completion.h"
#include <string.h>

/* GLSL keywords and types */
static const char *glsl_keywords[] = {
    "void", "bool", "int", "uint", "float", "double",
    "vec2", "vec3", "vec4", "dvec2", "dvec3", "dvec4",
    "bvec2", "bvec3", "bvec4", "ivec2", "ivec3", "ivec4",
    "uvec2", "uvec3", "uvec4",
    "mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4",
    "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4",
    "sampler2D", "sampler3D", "samplerCube",
    "if", "else", "for", "while", "do", "break", "continue", "return",
    "const", "uniform", "in", "out", "inout",
    "struct", "layout", "precision", "highp", "mediump", "lowp",
    "varying", "attribute",
    NULL
};

/* GLSL built-in functions */
static const char *glsl_functions[] = {
    /* Angle and trigonometry */
    "radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan",
    "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",

    /* Exponential */
    "pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt",

    /* Common */
    "abs", "sign", "floor", "ceil", "fract", "mod", "min", "max",
    "clamp", "mix", "step", "smoothstep",

    /* Geometric */
    "length", "distance", "dot", "cross", "normalize",
    "faceforward", "reflect", "refract",

    /* Matrix */
    "matrixCompMult", "transpose", "determinant", "inverse",

    /* Vector relational */
    "lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual",
    "equal", "notEqual", "any", "all", "not",

    /* Texture */
    "texture", "texture2D", "textureCube", "textureSize",
    "texelFetch", "textureLod",

    NULL
};

/* Shadertoy-specific uniforms and functions */
static const char *shadertoy_items[] = {
    /* Uniforms */
    "iTime", "iResolution", "iMouse", "iFrame", "iTimeDelta",
    "iFrameRate", "iChannelTime", "iChannelResolution",
    "iChannel0", "iChannel1", "iChannel2", "iChannel3",
    "iDate", "iSampleRate",

    /* Main function */
    "mainImage",

    NULL
};

/* Common shader snippets/patterns */
static const GLSLSnippet glsl_snippets[] = {
    {
        "mainImage",
        "void mainImage(out vec4 fragColor, in vec2 fragCoord)\n{\n\tvec2 uv = fragCoord / iResolution.xy;\n\tfragColor = vec4(uv, 0.5 + 0.5 * sin(iTime), 1.0);\n}",
        "Shadertoy main function template"
    },
    {
        "normalize_uv",
        "vec2 uv = (fragCoord * 2.0 - iResolution.xy) / iResolution.y;",
        "Normalize UV coordinates (centered, aspect-corrected)"
    },
    {
        "rotate2d",
        "mat2 rotate2d(float angle) {\n\tfloat s = sin(angle);\n\tfloat c = cos(angle);\n\treturn mat2(c, -s, s, c);\n}",
        "2D rotation matrix function"
    },
    {
        "palette",
        "vec3 palette(float t) {\n\tvec3 a = vec3(0.5, 0.5, 0.5);\n\tvec3 b = vec3(0.5, 0.5, 0.5);\n\tvec3 c = vec3(1.0, 1.0, 1.0);\n\tvec3 d = vec3(0.0, 0.33, 0.67);\n\treturn a + b * cos(6.28318 * (c * t + d));\n}",
        "Cosine-based color palette function"
    },
    { NULL, NULL, NULL }
};

/* Completion provider structure */
typedef struct {
    GObject parent;
    GList *proposals;
} GLSLCompletionProvider;

typedef struct {
    GObjectClass parent_class;
} GLSLCompletionProviderClass;

#define GLSL_TYPE_COMPLETION_PROVIDER (glsl_completion_provider_get_type())
#define GLSL_COMPLETION_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GLSL_TYPE_COMPLETION_PROVIDER, GLSLCompletionProvider))

static void glsl_completion_provider_iface_init(GtkSourceCompletionProviderIface *iface);

G_DEFINE_TYPE_WITH_CODE(
    GLSLCompletionProvider,
    glsl_completion_provider,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(GTK_SOURCE_TYPE_COMPLETION_PROVIDER, glsl_completion_provider_iface_init)
)

/* Create a completion proposal */
static GtkSourceCompletionItem* create_proposal(const char *label, const char *text, const char *info) {
    GtkSourceCompletionItem *item = gtk_source_completion_item_new();
    gtk_source_completion_item_set_label(item, label);
    gtk_source_completion_item_set_text(item, text);
    gtk_source_completion_item_set_info(item, info);
    return item;
}

/* Populate proposals list */
static void populate_proposals(GLSLCompletionProvider *provider) {
    if (provider->proposals) {
        return; /* Already populated */
    }

    /* Add keywords */
    for (int i = 0; glsl_keywords[i] != NULL; i++) {
        GtkSourceCompletionItem *item = create_proposal(
            glsl_keywords[i],
            glsl_keywords[i],
            "GLSL keyword"
        );
        provider->proposals = g_list_prepend(provider->proposals, item);
    }

    /* Add built-in functions */
    for (int i = 0; glsl_functions[i] != NULL; i++) {
        char text[128];
        snprintf(text, sizeof(text), "%s()", glsl_functions[i]);
        GtkSourceCompletionItem *item = create_proposal(
            glsl_functions[i],
            text,
            "GLSL built-in function"
        );
        provider->proposals = g_list_prepend(provider->proposals, item);
    }

    /* Add Shadertoy items */
    for (int i = 0; shadertoy_items[i] != NULL; i++) {
        GtkSourceCompletionItem *item = create_proposal(
            shadertoy_items[i],
            shadertoy_items[i],
            "Shadertoy uniform/function"
        );
        provider->proposals = g_list_prepend(provider->proposals, item);
    }

    /* Add snippets */
    for (int i = 0; glsl_snippets[i].label != NULL; i++) {
        GtkSourceCompletionItem *item = create_proposal(
            glsl_snippets[i].label,
            glsl_snippets[i].text,
            glsl_snippets[i].info
        );
        provider->proposals = g_list_prepend(provider->proposals, item);
    }

    /* Reverse to maintain order */
    provider->proposals = g_list_reverse(provider->proposals);
}

/* Forward declaration */
static void glsl_completion_provider_finalize(GObject *object);

/* GtkSourceCompletionProvider interface implementation */
static gchar* provider_get_name(GtkSourceCompletionProvider *provider) {
    (void)provider;
    return g_strdup("GLSL");
}

static void provider_populate(GtkSourceCompletionProvider *provider,
                              GtkSourceCompletionContext *context) {
    GLSLCompletionProvider *glsl_provider = GLSL_COMPLETION_PROVIDER(provider);
    GtkTextIter iter;

    populate_proposals(glsl_provider);

    if (!gtk_source_completion_context_get_iter(context, &iter)) {
        gtk_source_completion_context_add_proposals(context, provider, NULL, TRUE);
        return;
    }

    /* Get the word being typed */
    GtkTextIter start = iter;
    while (gtk_text_iter_backward_char(&start)) {
        gunichar c = gtk_text_iter_get_char(&start);
        if (!g_unichar_isalnum(c) && c != '_') {
            gtk_text_iter_forward_char(&start);
            break;
        }
    }

    gchar *word = gtk_text_iter_get_text(&start, &iter);
    gsize word_len = word ? strlen(word) : 0;

    /* Filter proposals based on prefix */
    GList *filtered = NULL;
    for (GList *item = glsl_provider->proposals; item != NULL; item = item->next) {
        GtkSourceCompletionItem *proposal = GTK_SOURCE_COMPLETION_ITEM(item->data);
        gchar *label = NULL;
        g_object_get(proposal, "label", &label, NULL);

        if (label && (word_len == 0 || g_str_has_prefix(label, word))) {
            filtered = g_list_prepend(filtered, proposal);
        }
        g_free(label);
    }

    g_free(word);
    filtered = g_list_reverse(filtered);

    gtk_source_completion_context_add_proposals(context, provider, filtered, TRUE);
    g_list_free(filtered);
}

static gboolean provider_match(GtkSourceCompletionProvider *provider,
                               GtkSourceCompletionContext *context) {
    GtkSourceCompletionActivation activation;
    GtkTextIter iter;
    GtkTextIter start;
    gchar *text;
    gboolean result = FALSE;

    (void)provider;

    activation = gtk_source_completion_context_get_activation(context);
    if (activation & GTK_SOURCE_COMPLETION_ACTIVATION_USER_REQUESTED) {
        return TRUE;
    }

    if (!gtk_source_completion_context_get_iter(context, &iter)) {
        return FALSE;
    }

    start = iter;
    /* Find start of word to check length */
    while (gtk_text_iter_backward_char(&start)) {
        gunichar c = gtk_text_iter_get_char(&start);
        if (!g_unichar_isalnum(c) && c != '_') {
            gtk_text_iter_forward_char(&start);
            break;
        }
    }

    text = gtk_text_iter_get_text(&start, &iter);
    if (text && strlen(text) >= 2) {
        result = TRUE;
    }
    g_free(text);

    return result;
}

static GtkSourceCompletionActivation provider_get_activation(GtkSourceCompletionProvider *provider) {
    (void)provider;
    return GTK_SOURCE_COMPLETION_ACTIVATION_INTERACTIVE |
           GTK_SOURCE_COMPLETION_ACTIVATION_USER_REQUESTED;
}

static void glsl_completion_provider_iface_init(GtkSourceCompletionProviderIface *iface) {
    iface->get_name = provider_get_name;
    iface->populate = provider_populate;
    iface->match = provider_match;
    iface->get_activation = provider_get_activation;
}

static void glsl_completion_provider_class_init(GLSLCompletionProviderClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = glsl_completion_provider_finalize;
}

static void glsl_completion_provider_init(GLSLCompletionProvider *provider) {
    provider->proposals = NULL;
}

static void glsl_completion_provider_finalize(GObject *object) {
    GLSLCompletionProvider *provider = GLSL_COMPLETION_PROVIDER(object);

    if (provider->proposals) {
        g_list_free_full(provider->proposals, g_object_unref);
        provider->proposals = NULL;
    }

    if (G_OBJECT_CLASS(glsl_completion_provider_parent_class)->finalize) {
        G_OBJECT_CLASS(glsl_completion_provider_parent_class)->finalize(object);
    }
}

/* Public API */
GtkSourceCompletionProvider* glsl_completion_provider_new(void) {
    GLSLCompletionProvider *provider = g_object_new(GLSL_TYPE_COMPLETION_PROVIDER, NULL);
    return GTK_SOURCE_COMPLETION_PROVIDER(provider);
}
