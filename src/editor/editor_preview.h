/* gleditor - OpenGL Preview Component
 * Handles live shader rendering and preview
 */

#ifndef EDITOR_PREVIEW_H
#define EDITOR_PREVIEW_H

#include <gtk/gtk.h>
#include <stdbool.h>

/**
 * Preview component context
 */
typedef struct {
    GtkWidget *gl_area;
    GtkWidget *container;
    unsigned int shader_program;
    unsigned int vao;
    unsigned int vbo;
    bool initialized;
    bool shader_valid;
    double start_time;
    bool paused;
    double pause_time;
    float mouse_x;
    float mouse_y;
    int fps;
    unsigned int animation_timer_id;
    void (*on_error)(const char *message, void *user_data);
    void *user_data;
} EditorPreview;

/**
 * Create preview component
 * @param fps Target frames per second
 * @return EditorPreview context
 */
EditorPreview *editor_preview_create(int fps);

/**
 * Destroy preview component
 * @param preview Preview context
 */
void editor_preview_destroy(EditorPreview *preview);

/**
 * Get the preview widget
 * @param preview Preview context
 * @return GtkWidget container
 */
GtkWidget *editor_preview_get_widget(EditorPreview *preview);

/**
 * Compile and load shader
 * @param preview Preview context
 * @param shader_source GLSL shader source code
 * @return true if successful, false on error
 */
bool editor_preview_compile_shader(EditorPreview *preview, const char *shader_source);

/**
 * Set pause state
 * @param preview Preview context
 * @param paused true to pause, false to resume
 */
void editor_preview_set_paused(EditorPreview *preview, bool paused);

/**
 * Check if paused
 * @param preview Preview context
 * @return true if paused
 */
bool editor_preview_is_paused(EditorPreview *preview);

/**
 * Reset animation time to zero
 * @param preview Preview context
 */
void editor_preview_reset_time(EditorPreview *preview);

/**
 * Set FPS
 * @param preview Preview context
 * @param fps Target frames per second
 */
void editor_preview_set_fps(EditorPreview *preview, int fps);

/**
 * Get current FPS
 * @param preview Preview context
 * @return Current FPS
 */
double editor_preview_get_current_fps(EditorPreview *preview);

/**
 * Set error callback
 * @param preview Preview context
 * @param callback Error callback function
 * @param user_data User data for callback
 */
void editor_preview_set_error_callback(EditorPreview *preview,
                                       void (*callback)(const char *, void *),
                                       void *user_data);

#endif /* EDITOR_PREVIEW_H */