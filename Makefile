# NeoWall Shader Editor - Standalone GTK Application
# Comprehensive build system with OpenGL ES detection

PROJECT := gleditor
VERSION := 1.0.0
PREFIX ?= /usr/local

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11
CFLAGS += -D_GNU_SOURCE -DVERSION=\"$(VERSION)\"
LDFLAGS :=
LIBS :=

# Directories
SRC_DIR := src
SHADER_LIB_DIR := $(SRC_DIR)/shader_lib
BUILD_DIR := build
BIN_DIR := bin

# Output binary
TARGET := $(BIN_DIR)/$(PROJECT)

# ============================================
# OpenGL ES / EGL Detection
# ============================================

# Detect EGL version
EGL_VERSION := $(shell pkg-config --modversion egl 2>/dev/null)
HAS_EGL := $(shell pkg-config --exists egl && echo yes)

# Detect OpenGL ES 1.x
HAS_GLES1_CM := $(shell pkg-config --exists glesv1_cm && echo yes)
HAS_GLES1 := $(shell test -f /usr/include/GLES/gl.h && echo yes)

# Detect OpenGL ES 2.0
HAS_GLES2 := $(shell pkg-config --exists glesv2 && echo yes)

# Detect OpenGL ES 3.0+
HAS_GLES3_HEADERS := $(shell test -f /usr/include/GLES3/gl3.h && echo yes)
HAS_GLES3_HEADERS_ALT := $(shell test -f /usr/include/GLES3/gl31.h && echo yes)
HAS_GLES3_HEADERS_ALT2 := $(shell test -f /usr/include/GLES3/gl32.h && echo yes)

# Check for specific GL ES 3.x versions
ifeq ($(HAS_GLES3_HEADERS),yes)
    HAS_GLES30 := yes
endif

ifeq ($(HAS_GLES3_HEADERS_ALT),yes)
    HAS_GLES31 := yes
    HAS_GLES30 := yes
endif

ifeq ($(HAS_GLES3_HEADERS_ALT2),yes)
    HAS_GLES32 := yes
    HAS_GLES31 := yes
    HAS_GLES30 := yes
endif

# Fallback: Try to detect via pkg-config
ifeq ($(HAS_GLES30),)
    HAS_GLES30 := $(shell pkg-config --exists glesv3 && echo yes)
endif

# ============================================
# Dependency Detection
# ============================================

# GTK+3 (required)
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0 2>/dev/null)
HAS_GTK := $(shell pkg-config --exists gtk+-3.0 && echo yes)

# GTKSourceView 4 (required for syntax highlighting)
GTKSOURCE_CFLAGS := $(shell pkg-config --cflags gtksourceview-4 2>/dev/null)
GTKSOURCE_LIBS := $(shell pkg-config --libs gtksourceview-4 2>/dev/null)
HAS_GTKSOURCE := $(shell pkg-config --exists gtksourceview-4 && echo yes)

# ============================================
# Configure OpenGL ES Support
# ============================================

# Validate dependencies
ifndef HAS_GTK
    $(error GTK+3 not found - required for shader editor GUI)
endif

ifndef HAS_GTKSOURCE
    $(error GTKSourceView 4 not found - required for code editor)
endif

ifndef HAS_EGL
    $(error EGL not found - required for OpenGL rendering)
endif

# OpenGL ES 1.x support (optional, for legacy compatibility)
ifeq ($(HAS_GLES1_CM),yes)
    CFLAGS += -DHAVE_GLES1
    LDFLAGS += -lGLESv1_CM
    GLES1_SUPPORT := yes
    $(info OpenGL ES 1.x detected (legacy support))
else
    GLES1_SUPPORT := no
    $(info OpenGL ES 1.x not found (optional))
endif

# OpenGL ES 2.0 support (required minimum)
ifeq ($(HAS_GLES2),yes)
    CFLAGS += -DHAVE_GLES2
    LDFLAGS += -lGLESv2
    GLES2_SUPPORT := yes
    $(info OpenGL ES 2.0 detected)
else
    $(error OpenGL ES 2.0 not found - minimum requirement)
endif

# OpenGL ES 3.0 support
ifeq ($(HAS_GLES30),yes)
    CFLAGS += -DHAVE_GLES3 -DHAVE_GLES30
    GLES30_SUPPORT := yes
    $(info OpenGL ES 3.0 detected (enhanced shader support))
else
    GLES30_SUPPORT := no
    $(info OpenGL ES 3.0 not found (shader compatibility limited))
endif

# OpenGL ES 3.1 support
ifeq ($(HAS_GLES31),yes)
    CFLAGS += -DHAVE_GLES31
    GLES31_SUPPORT := yes
    $(info OpenGL ES 3.1 detected (compute shader support))
else
    GLES31_SUPPORT := no
    $(info OpenGL ES 3.1 not found (optional))
endif

# OpenGL ES 3.2 support
ifeq ($(HAS_GLES32),yes)
    CFLAGS += -DHAVE_GLES32
    GLES32_SUPPORT := yes
    $(info OpenGL ES 3.2 detected (geometry/tessellation shaders))
else
    GLES32_SUPPORT := no
    $(info OpenGL ES 3.2 not found (optional))
endif

# Add GTK and GTKSourceView flags
CFLAGS += $(GTK_CFLAGS) $(GTKSOURCE_CFLAGS)
LIBS += $(GTK_LIBS) $(GTKSOURCE_LIBS)

# Add EGL
LDFLAGS += -lEGL

# Add math library
LDFLAGS += -lm

# ============================================
# Source Files
# ============================================

# Shader library sources
SHADER_LIB_SOURCES := $(SHADER_LIB_DIR)/shader_core.c \
                      $(SHADER_LIB_DIR)/shadertoy_compat.c \
                      $(SHADER_LIB_DIR)/shader_adaptation.c \
                      $(SHADER_LIB_DIR)/neowall_shader_api.c \
                      $(SHADER_LIB_DIR)/shader_utils.c

# Editor component sources
EDITOR_DIR := $(SRC_DIR)/editor
EDITOR_SOURCES := $(EDITOR_DIR)/editor_text.c \
                  $(EDITOR_DIR)/editor_preview.c \
                  $(EDITOR_DIR)/editor_toolbar.c \
                  $(EDITOR_DIR)/editor_statusbar.c \
                  $(EDITOR_DIR)/editor_error_panel.c \
                  $(EDITOR_DIR)/file_operations.c \
                  $(EDITOR_DIR)/editor_window.c \
                  $(EDITOR_DIR)/editor_settings.c

# Main application sources
APP_SOURCES := $(SRC_DIR)/main.c \
               $(SRC_DIR)/shader_editor.c

# All sources
SOURCES := $(SHADER_LIB_SOURCES) $(EDITOR_SOURCES) $(APP_SOURCES)

# Object files
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# ============================================
# Build Rules
# ============================================

.PHONY: all clean install uninstall info run debug help

all: info $(TARGET)

# Create directories
$(BUILD_DIR) $(BIN_DIR):
	@mkdir -p $@

$(BUILD_DIR)/shader_lib:
	@mkdir -p $@

$(BUILD_DIR)/editor:
	@mkdir -p $@

# Compile shader library
$(BUILD_DIR)/shader_lib/%.o: $(SHADER_LIB_DIR)/%.c | $(BUILD_DIR)/shader_lib
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile editor components
$(BUILD_DIR)/editor/%.o: $(EDITOR_DIR)/%.c | $(BUILD_DIR)/editor
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

# Compile main application
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	@echo "  LINK    $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	@echo ""
	@echo "✓ Build complete: $(TARGET)"
	@echo ""

# Run the application
run: $(TARGET)
	@echo "Running $(PROJECT)..."
	@$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG -O0
debug: clean all

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "✓ Clean complete"

# Install
install: $(TARGET)
	@echo "Installing $(PROJECT) to $(PREFIX)/bin..."
	@install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(PROJECT)
	@echo "✓ Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling $(PROJECT)..."
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(PROJECT)
	@echo "✓ Uninstall complete"

# Display build information
info:
	@echo "============================================"
	@echo "$(PROJECT) v$(VERSION) - Shader Editor"
	@echo "============================================"
	@echo ""
	@echo "OpenGL ES / EGL Support:"
	@echo "  EGL: $(if $(HAS_EGL),Yes (version $(EGL_VERSION)),No)"
	@echo "  OpenGL ES 1.x: $(if $(filter yes,$(GLES1_SUPPORT)),Yes,No)"
	@echo "  OpenGL ES 2.0: $(if $(filter yes,$(GLES2_SUPPORT)),Yes,No)"
	@echo "  OpenGL ES 3.0: $(if $(filter yes,$(GLES30_SUPPORT)),Yes,No)"
	@echo "  OpenGL ES 3.1: $(if $(filter yes,$(GLES31_SUPPORT)),Yes,No)"
	@echo "  OpenGL ES 3.2: $(if $(filter yes,$(GLES32_SUPPORT)),Yes,No)"
	@echo ""
	@echo "Dependencies:"
	@echo "  GTK+3: $(if $(HAS_GTK),Yes,No)"
	@echo "  GTKSourceView 4: $(if $(HAS_GTKSOURCE),Yes,No)"
	@echo ""
	@echo "Build Configuration:"
	@echo "  CC: $(CC)"
	@echo "  PREFIX: $(PREFIX)"
	@echo ""

# Help
help:
	@echo "NeoWall Shader Editor - Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build the shader editor"
	@echo "  make run          - Build and run the application"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make install      - Install to $(PREFIX)/bin"
	@echo "  make uninstall    - Remove installed binary"
	@echo "  make info         - Display build configuration"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Environment variables:"
	@echo "  PREFIX            - Installation prefix (default: /usr/local)"
	@echo "  CC                - C compiler (default: gcc)"
	@echo "  CFLAGS            - Additional compiler flags"
	@echo "  LDFLAGS           - Additional linker flags"
	@echo ""

# Dependencies
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -I$(SRC_DIR) -MMD -MP -c $< -o $@
