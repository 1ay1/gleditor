# CMakeLists.txt - Cross-platform build system for gleditor
cmake_minimum_required(VERSION 3.10)
project(gleditor VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    message(STATUS "Building for Windows")
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    message(STATUS "Building for macOS")
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    message(STATUS "Building for Linux")
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Include GNUInstallDirs early for CMAKE_INSTALL_DATADIR
include(GNUInstallDirs)

# Add version definition
add_definitions(-DVERSION="${PROJECT_VERSION}")

# Define data directory paths for installed location
add_definitions(-DGLEDITOR_DATADIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/gleditor")

# Find required packages
find_package(PkgConfig REQUIRED)

# GTK+3
pkg_check_modules(GTK3 REQUIRED gtk+-3.0>=3.20)
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

# GTKSourceView 4
pkg_check_modules(GTKSOURCEVIEW REQUIRED gtksourceview-4)
include_directories(${GTKSOURCEVIEW_INCLUDE_DIRS})
link_directories(${GTKSOURCEVIEW_LIBRARY_DIRS})
add_definitions(${GTKSOURCEVIEW_CFLAGS_OTHER})

# OpenGL ES / EGL detection
if(PLATFORM_LINUX OR PLATFORM_UNIX)
    # Linux/Unix: Use OpenGL ES + EGL
    pkg_check_modules(EGL REQUIRED egl)
    pkg_check_modules(GLES2 REQUIRED glesv2)

    include_directories(${EGL_INCLUDE_DIRS} ${GLES2_INCLUDE_DIRS})
    link_directories(${EGL_LIBRARY_DIRS} ${GLES2_LIBRARY_DIRS})

    set(GL_LIBRARIES ${EGL_LIBRARIES} ${GLES2_LIBRARIES})

    # Check for GLES3 support
    find_path(GLES3_INCLUDE_DIR GLES3/gl3.h)
    if(GLES3_INCLUDE_DIR)
        add_definitions(-DHAVE_GLES3)
        message(STATUS "OpenGL ES 3.0+ support detected")
    endif()

    add_definitions(-DHAVE_GLES2)

elseif(PLATFORM_MACOS)
    # macOS: Use OpenGL (not ES) with compatibility layer
    find_package(OpenGL REQUIRED)
    set(GL_LIBRARIES ${OPENGL_LIBRARIES})
    add_definitions(-DUSE_OPENGL_COMPAT)
    message(STATUS "Using OpenGL with ES compatibility layer on macOS")

elseif(PLATFORM_WINDOWS)
    # Windows: Use libepoxy for OpenGL function loading
    pkg_check_modules(EPOXY REQUIRED epoxy)
    include_directories(${EPOXY_INCLUDE_DIRS})
    link_directories(${EPOXY_LIBRARY_DIRS})
    set(GL_LIBRARIES ${EPOXY_LIBRARIES})
    add_definitions(-DUSE_EPOXY)
    message(STATUS "Using libepoxy for OpenGL on Windows")
endif()

# Math library (needed on some platforms)
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${MATH_LIBRARY})
endif()

# Source files
set(EDITOR_SOURCES
    src/editor/editor_window.c
    src/editor/editor_text.c
    src/editor/editor_preview.c
    src/editor/editor_toolbar.c
    src/editor/editor_statusbar.c
    src/editor/editor_settings.c
    src/editor/editor_tabs.c
    src/editor/editor_templates.c
    src/editor/editor_error_panel.c
    src/editor/editor_help.c
    src/editor/file_operations.c
    src/editor/glsl_completion.c
    src/editor/keyboard_shortcuts.c
)

set(SHADER_LIB_SOURCES
    src/shader_lib/neowall_shader_api.c
    src/shader_lib/shader_core.c
    src/shader_lib/shadertoy_compat.c
    src/shader_lib/shader_adaptation.c
)

set(MAIN_SOURCE
    src/main.c
    src/shader_editor.c
)

# Create executable
add_executable(gleditor
    ${MAIN_SOURCE}
    ${EDITOR_SOURCES}
    ${SHADER_LIB_SOURCES}
)

# Include directories
target_include_directories(gleditor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/editor
    ${CMAKE_SOURCE_DIR}/src/shader_lib
)

# Link libraries
target_link_libraries(gleditor
    ${GTK3_LIBRARIES}
    ${GTKSOURCEVIEW_LIBRARIES}
    ${GL_LIBRARIES}
    ${EXTRA_LIBS}
)

# Platform-specific settings
if(PLATFORM_WINDOWS)
    # Windows: Add console flag for debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(gleditor PROPERTIES
            WIN32_EXECUTABLE FALSE
        )
    else()
        set_target_properties(gleditor PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    endif()

    # Add Windows resource file if available
    if(EXISTS "${CMAKE_SOURCE_DIR}/data/gleditor.rc")
        target_sources(gleditor PRIVATE data/gleditor.rc)
    endif()

elseif(PLATFORM_MACOS)
    # macOS: Create app bundle
    set_target_properties(gleditor PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/data/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "gleditor"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

# Installation
# (GNUInstallDirs already included above)

# Install binary
install(TARGETS gleditor
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION /Applications
)

# Install data files
install(FILES
    data/gleditor.svg
    data/icon.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/gleditor
)

# Install desktop entry (Linux only)
if(PLATFORM_LINUX)
    install(FILES data/gleditor.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )

    install(FILES data/icon.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME gleditor.svg
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "==============================================")
message(STATUS "gleditor v${PROJECT_VERSION} - Configuration Summary")
message(STATUS "==============================================")
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  GTK+3: ${GTK3_VERSION}")
message(STATUS "  GTKSourceView: ${GTKSOURCEVIEW_VERSION}")
if(PLATFORM_LINUX)
    message(STATUS "  OpenGL ES: ${GLES2_VERSION}")
    message(STATUS "  EGL: ${EGL_VERSION}")
else()
    message(STATUS "  OpenGL: Found (with ES compatibility)")
endif()
message(STATUS "==============================================")
message(STATUS "")
