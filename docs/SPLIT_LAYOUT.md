# Split Layout Feature - Editor and Preview Orientation

## Overview
The Split Layout feature allows you to configure how the editor and preview windows are arranged - either side-by-side (horizontal) or top-and-bottom (vertical).

## Feature Details

### Layout Options

#### 📐 Horizontal Layout (Side by Side)
- **Description**: Editor on the left, preview on the right
- **Best For**:
  - Wide-screen monitors (16:9, 21:9)
  - Desktop workflows
  - When you want maximum vertical space for code
  - Working with taller shaders
- **Default**: Yes

#### 📐 Vertical Layout (Top and Bottom)
- **Description**: Editor on top, preview on bottom
- **Best For**:
  - Tall/portrait monitors
  - Laptop screens with limited width
  - When previewing wide aspect ratio content
  - Working with shorter shaders

### How to Change Layout

1. Click the **⚙️ Settings** button in the toolbar (or press `Ctrl+,`)
2. Find **📐 Split Layout** setting
3. Select your preferred layout:
   - "Horizontal (Side by Side)"
   - "Vertical (Top and Bottom)"
4. Layout changes **instantly** - no restart required!

### Technical Behavior

- **Instant Apply**: Changes take effect immediately when selected
- **Maintains Ratio**: Switches between orientations while keeping 50/50 split
- **Persists**: Your choice is saved to `~/.config/gleditor/settings.conf`
- **Smooth Transition**: Children widgets are safely re-parented to new orientation

## Implementation Details

### Settings Structure
```c
typedef enum {
    SPLIT_HORIZONTAL = 0,  // Side by side
    SPLIT_VERTICAL = 1      // Top and bottom
} SplitOrientation;

typedef struct {
    // ... other settings ...
    SplitOrientation split_orientation;
} EditorSettings;
```

### How It Works

1. **Initialization**: Window creates `GtkPaned` with orientation from settings
2. **Change Detection**: Settings callback detects orientation change
3. **Safe Transition**:
   - Reference children widgets (editor + preview)
   - Remove from current paned
   - Change paned orientation
   - Re-add children
   - Recalculate split position (50/50)
4. **Persistence**: Choice saved to config file automatically

### Code Location

- **Header**: `src/editor/editor_settings.h` - `SplitOrientation` enum
- **Settings**: `src/editor/editor_settings.c` - Save/load/dialog
- **Window**: `src/editor/editor_window.c` - Orientation change logic

## Usage Examples

### For Wide Monitors
```
Recommended: Horizontal (Side by Side)
┌─────────────────────────────────────┐
│  Editor  │       Preview           │
│  (code)  │     (shader output)     │
│          │                          │
│          │                          │
└─────────────────────────────────────┘
```

### For Tall/Narrow Monitors
```
Recommended: Vertical (Top and Bottom)
┌─────────────────────┐
│      Editor         │
│      (code)         │
├─────────────────────┤
│      Preview        │
│  (shader output)    │
└─────────────────────┘
```

## Benefits

1. **Flexibility**: Adapt to different screen sizes and ratios
2. **Productivity**: Choose layout that maximizes your visible code/preview area
3. **No Restart**: Switch on-the-fly as needed
4. **Persistent**: Remembers your preference
5. **Simple**: Just two clicks to change

## Testing

To verify the feature works correctly:

- [ ] Open Settings dialog
- [ ] See "📐 Split Layout" option
- [ ] Change from Horizontal to Vertical
- [ ] Layout changes immediately without closing dialog
- [ ] Editor and preview maintain 50/50 split
- [ ] Close and reopen application
- [ ] Layout setting persisted (stays as chosen)
- [ ] Switch back to Horizontal
- [ ] Works smoothly both directions

## Future Enhancements

Possible improvements:
- [ ] Custom split ratios (e.g., 60/40, 70/30)
- [ ] Save split position per orientation
- [ ] Keyboard shortcut to toggle orientation (`Ctrl+L`?)
- [ ] Three-panel layout option (editor, preview, documentation)
- [ ] Remember different splits for different files

## Configuration File

Setting stored in `~/.config/gleditor/settings.conf`:
```ini
# 0 = Horizontal (side by side)
# 1 = Vertical (top and bottom)
split_orientation=0
```

## Version History

- **v1.0.1** (2024): Split layout feature added
  - Horizontal/Vertical orientation support
  - Instant switching
  - Settings persistence

---

**Feature Status**: ✅ Implemented and Working  
**Default**: Horizontal (Side by Side)  
**Shortcut**: None (via Settings dialog)