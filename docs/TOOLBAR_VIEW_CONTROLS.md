# Toolbar View Controls - Quick Layout Management

## Overview
The toolbar now includes quick-access view control buttons that allow you to instantly change the split orientation and toggle visibility of the editor and preview panels.

---

## 🎛️ View Control Buttons

### 📐 Split Orientation Toggle (⇆)
**Location**: Toolbar, after shader controls

**Function**: Toggles between horizontal and vertical split layouts

**Behavior**:
- **Click once**: Switches from horizontal to vertical (or vice versa)
- **Icon changes**: 
  - `⇆` (left-right arrows) for horizontal split
  - `⇅` (top-bottom arrows) for vertical split
- **Instant apply**: No restart needed
- **Persists**: Choice is saved to settings automatically
- **Maintains 50/50 split** when switching

**Use Cases**:
- Quickly adapt to different screen orientations
- Switch layouts without opening settings dialog
- One-click workflow optimization

---

### 📝 Editor Visibility Toggle
**Location**: Toolbar, after split toggle

**Function**: Shows/hides the text editor panel

**Behavior**:
- **Click to hide editor**: Focus entirely on shader preview
- **Click to show editor**: Bring editor back
- **Visual feedback**: Button dims (40% opacity) when editor is hidden
- **Safety**: Cannot hide both editor and preview at the same time
- **Status message**: Shows "Editor hidden" or "Editor shown"

**Use Cases**:
- **Preview-only mode**: Hide editor to maximize preview for presentations
- **Fullscreen preview**: Perfect for showcasing shader output
- **Screenshot mode**: Capture clean preview without code

---

### 🎬 Preview Visibility Toggle
**Location**: Toolbar, after editor toggle

**Function**: Shows/hides the shader preview panel

**Behavior**:
- **Click to hide preview**: Focus entirely on editing code
- **Click to show preview**: Bring preview back
- **Visual feedback**: Button dims (40% opacity) when preview is hidden
- **Safety**: Cannot hide both editor and preview at the same time
- **Status message**: Shows "Preview hidden" or "Preview shown"

**Use Cases**:
- **Code-only mode**: Maximize editor space for complex shaders
- **Low-resource mode**: Reduce GPU usage while editing
- **Planning mode**: Work on shader logic without visual distraction

---

## 🎯 Common Workflows

### Presentation Mode
1. Click **📝 Editor Toggle** to hide editor
2. Only shader preview visible
3. Perfect for showcasing your work

### Focused Coding Mode
1. Click **🎬 Preview Toggle** to hide preview
2. Full screen for code
3. Great for writing complex logic

### Portrait/Narrow Screen
1. Click **📐 Split Toggle** to switch to vertical
2. Editor on top, preview on bottom
3. Better use of vertical space

### Wide Screen
1. Click **📐 Split Toggle** to switch to horizontal
2. Editor on left, preview on right
3. Better use of horizontal space

### Quick Layout Switching
- No need to open settings dialog
- One click to change orientation
- Instant visual feedback
- Settings persist across restarts

---

## 🔧 Technical Details

### Split Orientation
- **Horizontal (Default)**: `GtkPaned` with `GTK_ORIENTATION_HORIZONTAL`
- **Vertical**: `GtkPaned` with `GTK_ORIENTATION_VERTICAL`
- **Position**: Always maintains 50/50 split on toggle
- **Saves to**: `~/.config/gleditor/settings.conf` (`split_orientation=0` or `1`)

### Visibility Toggles
- Uses `gtk_widget_set_visible()` to show/hide panels
- Panels remain in memory (state preserved)
- Opacity feedback via `gtk_widget_set_opacity()`
- Mutex logic prevents hiding both panels

### Button Icons
- Split button uses system icons: `view-split-left-right`, `view-split-top-bottom`
- Editor button uses: `text-x-generic` with 📝 emoji
- Preview button uses: `video-display` with 🎬 emoji
- All buttons have tooltips for clarity

---

## 📋 Button Reference

| Button | Icon | Tooltip | Action | Shortcut |
|--------|------|---------|--------|----------|
| Split Toggle | ⇆/⇅ | Toggle Split Orientation | Horizontal ↔ Vertical | None |
| Editor Toggle | 📝 | Toggle Editor Visibility | Show/Hide Editor | None |
| Preview Toggle | 🎬 | Toggle Preview Visibility | Show/Hide Preview | None |

---

## 💡 Tips & Best Practices

1. **Screen Recording**: Hide editor for clean shader recordings
2. **Debugging**: Hide preview to reduce GPU load during complex edits
3. **Presentations**: One-click to hide code, show only output
4. **Mobile/Tablet**: Use vertical split for better aspect ratio
5. **Ultrawide Monitors**: Use horizontal split to maximize both panels
6. **Performance**: Hide preview when not needed to save resources
7. **Focus**: Toggle panels to reduce distractions

---

## 🚨 Safety Features

### Cannot Hide Both Panels
If you try to hide the last visible panel, you'll see:
```
Status Bar: "Cannot hide both editor and preview"
```

**Why?**: At least one panel must be visible for the application to be useful.

**Solution**: Show one panel before hiding the other.

---

## 🔄 Integration with Settings Dialog

The toolbar buttons and settings dialog are synchronized:

- **Toolbar split toggle** ↔️ **Settings → Split Layout dropdown**
- Changes in either location update both
- All changes persist to config file
- No conflicts or desyncs

---

## 🎨 Visual Feedback

### Split Button
- Icon changes to match current orientation
- Always shows what will happen on next click

### Visibility Buttons
- **Full opacity (1.0)**: Panel is visible
- **Dimmed (0.4)**: Panel is hidden
- Instant visual confirmation of state

### Status Bar
- Shows confirmation messages for all actions
- Brief, clear feedback
- No popups or interruptions

---

## 🧪 Testing

To verify the feature works:

- [ ] Click split toggle - orientation changes instantly
- [ ] Click again - switches back
- [ ] Icon updates to match orientation
- [ ] Hide editor - editor disappears, button dims
- [ ] Show editor - editor returns, button full opacity
- [ ] Hide preview - preview disappears, button dims
- [ ] Show preview - preview returns, button full opacity
- [ ] Try hiding both - prevented with status message
- [ ] Restart app - settings persist

---

## 🆚 Toolbar vs Settings Dialog

### Use Toolbar When:
- ✅ Quick one-click changes during work
- ✅ Frequently switching layouts
- ✅ Presenting or recording
- ✅ Need instant visibility toggles

### Use Settings Dialog When:
- ⚙️ Changing multiple settings at once
- ⚙️ Setting up initial preferences
- ⚙️ Want to see all options together
- ⚙️ Adjusting font, speed, auto-compile, etc.

---

## 🔮 Future Enhancements

Possible additions:
- [ ] Keyboard shortcuts (e.g., `F6` for split toggle)
- [ ] Remember last visibility state
- [ ] Custom split ratios (60/40, 70/30)
- [ ] Three-panel mode (editor, preview, console)
- [ ] Drag-to-reorder panels

---

## 📝 Summary

**Added in v1.0.1**:
- 🎛️ Three new toolbar buttons for instant layout control
- 📐 Split orientation toggle (horizontal/vertical)
- 📝 Editor visibility toggle
- 🎬 Preview visibility toggle
- 🔒 Safety to prevent hiding both panels
- 💾 All changes persist automatically
- ✨ Instant visual feedback

**Benefits**:
- No need to open settings for common layout changes
- One-click workflow optimization
- Better screen space utilization
- Flexible presentation modes
- Distraction-free coding or previewing

---

**Feature Status**: ✅ Fully Implemented  
**Location**: Toolbar (View Controls section)  
**Persistence**: Automatic  
**Shortcuts**: None (toolbar only)  
**Version**: 1.0.1+