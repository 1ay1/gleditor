/* OpenGL Preview Component - Header
 * Handles the live shader preview rendering
 */

#ifndef EDITOR_PREVIEW_H
#define EDITOR_PREVIEW_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "platform_compat.h"

/* Preview state callback signatures */
typedef void (*editor_preview_error_callback_t)(const char *error, gpointer user_data);
typedef void (*editor_preview_double_click_callback_t)(gpointer user_data);

/**
 * Create the OpenGL preview widget
 * 
 * @return The GL area widget
 */
GtkWidget *editor_preview_create(void);

/**
 * Compile and update the shader program
 * 
 * @param shader_code GLSL shader source code
 * @return true if compilation succeeded, false otherwise
 */
bool editor_preview_compile_shader(const char *shader_code);

/**
 * Get the last compilation error message
 * 
 * @return Error message (static, do not free) or NULL if no error
 */
const char *editor_preview_get_error(void);

/**
 * Check if a shader is currently loaded
 * 
 * @return true if a valid shader is loaded
 */
bool editor_preview_has_shader(void);

/**
 * Set animation paused state
 * 
 * @param paused true to pause, false to resume
 */
void editor_preview_set_paused(bool paused);

/**
 * Check if animation is paused
 * 
 * @return true if paused
 */
bool editor_preview_is_paused(void);

/**
 * Set animation speed multiplier
 * 
 * @param speed Speed multiplier (1.0 = normal, 2.0 = double speed, etc.)
 */
void editor_preview_set_speed(float speed);

/**
 * Get animation speed multiplier
 * 
 * @return Current speed multiplier
 */
float editor_preview_get_speed(void);

/**
 * Reset shader time to zero
 */
void editor_preview_reset_time(void);

/**
 * Get current FPS
 * 
 * @return Frames per second
 */
double editor_preview_get_fps(void);

/**
 * Get mouse position in normalized coordinates
 * 
 * @param x Output: X coordinate (0.0 to 1.0)
 * @param y Output: Y coordinate (0.0 to 1.0)
 */
void editor_preview_get_mouse(float *x, float *y);

/**
 * Set error callback
 * Called when shader compilation fails
 * 
 * @param callback Callback function
 * @param user_data User data passed to callback
 */
void editor_preview_set_error_callback(editor_preview_error_callback_t callback,
                                       gpointer user_data);

/**
 * Set double-click callback
 * Called when preview is double-clicked (to toggle fullscreen)
 * 
 * @param callback Callback function
 * @param user_data User data passed to callback
 */
void editor_preview_set_double_click_callback(editor_preview_double_click_callback_t callback,
                                               gpointer user_data);

/**
 * Force a redraw of the preview
 */
void editor_preview_queue_render(void);

/**
 * Destroy the preview and free resources
 */
void editor_preview_destroy(void);

#endif /* EDITOR_PREVIEW_H */