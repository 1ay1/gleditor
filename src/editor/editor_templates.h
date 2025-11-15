/* Template Selection Dialog Module
 * Provides a dialog for selecting shader templates
 */

#ifndef EDITOR_TEMPLATES_H
#define EDITOR_TEMPLATES_H

#include <gtk/gtk.h>
#include <stdbool.h>

/**
 * Template info structure
 */
typedef struct {
    const char *name;
    const char *display_name;
    const char *description;
    const char *code;
} TemplateInfo;

/**
 * Show template selection dialog
 * 
 * @param parent Parent window for the dialog
 * @return Selected template code, or NULL if cancelled
 */
char *editor_templates_show_dialog(GtkWindow *parent);

/**
 * Get list of available templates
 * 
 * @param count Output parameter for number of templates
 * @return Array of template info structures
 */
const TemplateInfo *editor_templates_get_list(size_t *count);

/**
 * Get template code by name
 * 
 * @param name Template name
 * @return Template code, or NULL if not found
 */
const char *editor_templates_get_code(const char *name);

#endif /* EDITOR_TEMPLATES_H */