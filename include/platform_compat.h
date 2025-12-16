/* Cross-Platform Compatibility Layer
 * Handles platform-specific differences for Windows, macOS, and Linux
 */

#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Platform detection */
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #define PLATFORM_NAME "Windows"
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
    #define PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define PLATFORM_LINUX
    #define PLATFORM_NAME "Linux"
#elif defined(__unix__)
    #define PLATFORM_UNIX
    #define PLATFORM_NAME "Unix"
#else
    #define PLATFORM_UNKNOWN
    #define PLATFORM_NAME "Unknown"
#endif

/* ============================================
 * Unified OpenGL Header Include
 * ============================================ */
#ifdef USE_EPOXY
    /* Windows/macOS: Use libepoxy for function loading */
    #include <epoxy/gl.h>
#elif defined(HAVE_GLES3)
    /* Linux: OpenGL ES 3.x */
    #include <GLES3/gl3.h>
#elif defined(HAVE_GLES2)
    /* Linux: OpenGL ES 2.0 */
    #include <GLES2/gl2.h>
#else
    /* Fallback: Desktop OpenGL */
    #include <GL/gl.h>
#endif

/* Windows-specific includes and definitions */
#ifdef PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
    #include <sys/stat.h>
    
    /* Map POSIX functions to Windows equivalents */
    #define mkdir(path, mode) _mkdir(path)
    #define access(path, mode) _access(path, mode)
    #define getcwd _getcwd
    #ifndef PATH_MAX
    #define PATH_MAX MAX_PATH
    #endif
    #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
    
    /* Directory separator */
    #define PATH_SEPARATOR '\\'
    #define PATH_SEPARATOR_STR "\\"
    
#else
    /* POSIX systems (Linux, macOS, Unix) */
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/time.h>
    #include <limits.h>
    
    /* Directory separator */
    #define PATH_SEPARATOR '/'
    #define PATH_SEPARATOR_STR "/"
#endif

/* Config directory helpers */
#ifdef PLATFORM_WINDOWS
    #define CONFIG_DIR_ENV "APPDATA"
    #define CONFIG_SUBDIR "gleditor"
#elif defined(PLATFORM_MACOS)
    #define CONFIG_DIR_ENV "HOME"
    #define CONFIG_SUBDIR "Library/Application Support/gleditor"
#else
    #define CONFIG_DIR_ENV "HOME"
    #define CONFIG_SUBDIR ".config/gleditor"
#endif

/* Cross-platform time functions */
static inline double platform_get_time(void) {
#ifdef PLATFORM_WINDOWS
    static LARGE_INTEGER frequency;
    static bool initialized = false;
    
    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = true;
    }
    
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
#endif
}

/* Cross-platform sleep (milliseconds) */
static inline void platform_sleep_ms(unsigned int milliseconds) {
#ifdef PLATFORM_WINDOWS
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

/* Cross-platform directory creation */
static inline int platform_mkdir_recursive(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    if (tmp[len - 1] == PATH_SEPARATOR) {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == PATH_SEPARATOR) {
            *p = 0;
#ifdef PLATFORM_WINDOWS
            _mkdir(tmp);
#else
            mkdir(tmp, 0755);
#endif
            *p = PATH_SEPARATOR;
        }
    }
    
#ifdef PLATFORM_WINDOWS
    return _mkdir(tmp);
#else
    return mkdir(tmp, 0755);
#endif
}

/* Cross-platform path utilities */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
static inline void platform_path_join(char *dest, size_t dest_size, const char *path1, const char *path2) {
    snprintf(dest, dest_size, "%s%c%s", path1, PATH_SEPARATOR, path2);
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static inline const char* platform_get_home_dir(void) {
#ifdef PLATFORM_WINDOWS
    return getenv("USERPROFILE");
#else
    return getenv("HOME");
#endif
}

static inline void platform_get_config_dir(char *dest, size_t dest_size) {
    const char *home = platform_get_home_dir();
    if (home) {
        platform_path_join(dest, dest_size, home, CONFIG_SUBDIR);
    } else {
        snprintf(dest, dest_size, CONFIG_SUBDIR);
    }
}

/* Cross-platform file existence check */
static inline bool platform_file_exists(const char *path) {
#ifdef PLATFORM_WINDOWS
    return _access(path, 0) == 0;
#else
    return access(path, F_OK) == 0;
#endif
}

/* Cross-platform directory check */
static inline bool platform_is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

#endif /* PLATFORM_COMPAT_H */