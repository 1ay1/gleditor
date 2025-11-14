#ifndef GLSL_COMPLETION_H
#define GLSL_COMPLETION_H

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

/* GLSL snippet structure */
typedef struct {
    const char *label;
    const char *text;
    const char *info;
} GLSLSnippet;

/**
 * Create a new GLSL completion provider
 * 
 * @return A new GtkSourceCompletionProvider for GLSL
 */
GtkSourceCompletionProvider* glsl_completion_provider_new(void);

#endif /* GLSL_COMPLETION_H */