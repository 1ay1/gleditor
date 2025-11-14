#ifndef EDITOR_HELP_H
#define EDITOR_HELP_H

#include <gtk/gtk.h>

/**
 * Show the help dialog with keyboard shortcuts, tips, and GLSL reference
 * 
 * @param parent Parent window for the dialog
 */
void editor_help_show_dialog(GtkWindow *parent);

#endif /* EDITOR_HELP_H */