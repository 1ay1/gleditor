/* NeoWall Shader Editor - Thin Wrapper
 * Provides backward compatibility with the new modular architecture
 */

#include "shader_editor.h"
#include "editor/editor_window.h"

/**
 * Show the shader editor dialog
 * This is now a thin wrapper around the modular editor_window component
 */
void shader_editor_show(GtkApplication *app) {
    editor_window_show(app);
}

/**
 * Close the shader editor dialog if open
 */
void shader_editor_close(void) {
    editor_window_close();
}

/**
 * Check if shader editor is currently open
 */
bool shader_editor_is_open(void) {
    return editor_window_is_open();
}
