# Panel Subsystem

This document describes the new Turtle panel implementation, how it integrates with the window manager, and what changed in the update.

## What changed

- Added a new bottom panel module under `ui/panel`.
- Integrated the panel into the window manager so the panel is rendered on startup and during redraws.
- Added panel-style loading support and panel input handling for mouse clicks and keyboard events.
- Updated the build system to compile panel sources and include the new `ui/panel/include` directory.

## Panel responsibilities

The new panel module provides:

- a bottom taskbar-style panel rendered at the bottom of the desktop
- quick-launch buttons for Turtle, calendar, browser, calculator, battery, and shutdown
- a search bar with an embedded find icon
- live system stats displayed before the clock: CPU usage, memory usage, and process count
- click handling for panel interactions
- keyboard input handling when the search bar is focused

## Files added or updated

- `ui/panel/include/panel.hpp`
- `ui/panel/src/panel.cpp`
- `ui/panel/include/turtle_button.hpp`
- `ui/panel/src/turtle_button.cpp`
- `ui/panel/include/calendar_button.hpp`
- `ui/panel/src/calendar_button.cpp`
- `ui/panel/include/browser_button.hpp`
- `ui/panel/src/browser_button.cpp`
- `ui/panel/include/calculator_button.hpp`
- `ui/panel/src/calculator_button.cpp`
- `ui/panel/include/battery_button.hpp`
- `ui/panel/src/battery_button.cpp`
- `ui/panel/include/shutdown_button.hpp`
- `ui/panel/src/shutdown_button.cpp`
- `ui/panel/include/panel_placeholder.hpp`
- `ui/panel/src/panel_placeholder.cpp`

## How it integrates with the window manager

The window manager now owns a `panel::Panel` instance in `ui/wm/include/window_manager.hpp`.

Startup flow (`WindowManager::initialize`):

1. Initialize display, desktop window, cursor renderer, and background.
2. Load panel style data from `style/panel/panel.style`.
3. Render the wallpaper into the desktop window.
4. Draw the panel on top of the wallpaper.
5. Capture the desktop content into pixmaps for later cursor and redraw restoration.

Redraw flow (`WindowManager::render_frame`):

- All drawing is double-buffered: the wallpaper, panel, and right-click menu are composed into an off-screen `frame_pixmap_` first.
- The completed frame is then presented to the window in a single `XCopyArea` blit.
- The software cursor overlay is drawn on top after presentation.
- This eliminates visible flicker (e.g. the search-bar blink) that previously occurred when stale content was momentarily shown on screen.

Input handling updates:

- Mouse clicks on the panel are forwarded to `Panel::handle_mouse_press`.
- Key presses are forwarded to `Panel::handle_key_press` when the search bar is focused.
- Panel redraws trigger `render_frame()` so the panel remains consistent.

## Style and rendering behavior

- The panel is drawn as a rounded rectangle at the bottom of the desktop.
- The search bar is rendered with a consistent background color and a find icon.
- The search bar no longer uses hover-based sky-blue fill when focused.
- The panel now updates cleanly on typing without visible blinking, thanks to double-buffered rendering.

## Live system stats

The panel displays three live system metrics positioned between the placeholder area and the clock:

```
... | placeholder | cpu: X% | mem: X% | processes: N | HH:MM | 🔋 | ⏻ |
```

### Data sources

| Stat | Source | Method |
|------|--------|--------|
| CPU % | `/proc/stat` | Computes delta between consecutive reads of the aggregate CPU line. Uses `static` locals to track previous idle/total values across calls. |
| Memory % | `/proc/meminfo` | Reads `MemTotal` and `MemAvailable`, computes `100 * (total - available) / total`. |
| Processes | `/proc` | Counts directories with numeric names (each represents a running process). |

### Rendering

- Stats are drawn in a subtle blueish-grey color (`0xAAAAAAFF`) to visually distinguish them from the white clock text.
- The placeholder area shrinks automatically to accommodate the stats block.
- Values update on every panel redraw (triggered by input events or expose events).

## Build system updates

The Makefile now includes `ui/panel/src` files in `SRC_FILES` and adds `ui/panel/include` to `INCLUDE_DIRS`.

This ensures the new panel module is compiled and linked with the window manager.

## Notes

- The panel module displays live system information and can be extended later with live task icons, notification areas, and interactive workspace elements.
- The modular design keeps panel rendering and input handling separate from the main window manager event loop.
- CPU usage reads will return 0% on the very first call since there is no previous delta; subsequent redraws converge to the real value.
