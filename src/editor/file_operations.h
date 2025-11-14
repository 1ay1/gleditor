/* File Operations Component - Header
 * Handles file dialogs and shader file operations
 */

#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <gtk/gtk.h>
#include <stdbool.h>

/**
 * Show file chooser dialog for loading a shader
 * 
 * @param parent Parent window
 * @return Selected file path (caller must free) or NULL if cancelled
 */
char *file_operations_load_dialog(GtkWindow *parent);

/**
 * Show file chooser dialog for saving a shader
 * 
 * @param parent Parent window
 * @param current_path Current file path (or NULL)
 * @return Selected file path (caller must free) or NULL if cancelled
 */
char *file_operations_save_dialog(GtkWindow *parent, const char *current_path);

/**
 * Load shader code from file
 * 
 * @param path File path
 * @param error Output: error message (caller must free) or NULL
 * @return Shader code (caller must free) or NULL on error
 */
char *file_operations_load_file(const char *path, char **error);

/**
 * Save shader code to file
 * 
 * @param path File path
 * @param code Shader code
 * @param error Output: error message (caller must free) or NULL
 * @return true on success, false on error
 */
bool file_operations_save_file(const char *path, const char *code, char **error);

/**
 * Get NeoWall shader directory path
 * Creates the directory if it doesn't exist
 * 
 * @return Directory path (caller must free) or NULL on error
 */
char *file_operations_get_neowall_shader_dir(void);

/**
 * Install shader to NeoWall
 * Copies the shader file to NeoWall's shader directory
 * 
 * @param shader_code Shader code to install
 * @param shader_name Shader name (without extension)
 * @param error Output: error message (caller must free) or NULL
 * @return true on success, false on error
 */
bool file_operations_install_to_neowall(const char *shader_code, 
                                        const char *shader_name,
                                        char **error);

/**
 * Check if a file exists
 * 
 * @param path File path
 * @return true if file exists, false otherwise
 */
bool file_operations_file_exists(const char *path);

/**
 * Get file name from path (without directory)
 * 
 * @param path Full file path
 * @return File name (static, do not free)
 */
const char *file_operations_get_filename(const char *path);

/**
 * Get file extension from path
 * 
 * @param path File path
 * @return Extension (static, do not free) or NULL if no extension
 */
const char *file_operations_get_extension(const char *path);

/**
 * Show confirmation dialog
 * 
 * @param parent Parent window
 * @param title Dialog title
 * @param message Dialog message
 * @return true if user confirmed, false if cancelled
 */
bool file_operations_confirm_dialog(GtkWindow *parent,
                                    const char *title,
                                    const char *message);

/**
 * Show error dialog
 * 
 * @param parent Parent window
 * @param title Dialog title
 * @param message Error message
 */
void file_operations_error_dialog(GtkWindow *parent,
                                  const char *title,
                                  const char *message);

/**
 * Show info dialog
 * 
 * @param parent Parent window
 * @param title Dialog title
 * @param message Info message
 */
void file_operations_info_dialog(GtkWindow *parent,
                                 const char *title,
                                 const char *message);

#endif /* FILE_OPERATIONS_H */