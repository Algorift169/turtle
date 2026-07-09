# UI Interaction and Cursor Style

This document describes the window manager's right-click menu and cursor implementation.

## Right-click menu (`ui/tabs`)

The new right-click desktop menu is implemented in:

- `ui/tabs/include/w-tab.hpp`
- `ui/tabs/src/w-tab.cpp`

### Behavior

- Right-click opens the menu at the pointer location.
- The menu remains visible while the mouse moves over it.
- Clicking outside the menu closes it.
- Clicking a menu item currently closes the menu; action handling can be added later.

### Visual style

- The menu uses a dark translucent rounded background.
- Menu items are rendered as text only, without boxes or separators.
- The panel is sized to fit the item labels with a compact layout.

## Cursor Style (`ui/cursor`)

The custom cursor is defined in:

- `ui/cursor/include/cursor.hpp`
- `ui/cursor/src/cursor.cpp`

The software cursor is drawn as an overlay into the desktop window and does not modify the system cursor.

### Cursor states

- `Normal`
- `LeftClick`
- `RightClick`
- `ScrollUp`
- `ScrollDown`

### Style definition

A style file is available at `style/cursor/cursor.style` for future theming and cursor configuration.

## Mouse and redraw behavior

The window manager maintains a `background_pixmap` snapshot of the wallpaper-only desktop. When the cursor moves or a menu is open, the manager restores only the previous cursor region before drawing the cursor and menu on top.

This prevents artifacts and minimizes redraw flicker.
