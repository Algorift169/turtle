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

- Restore the wallpaper snapshot when possible.
- Draw the panel again after the wallpaper is restored.
- Draw any right-click menu overlay.
- Draw the software cursor overlay.

Input handling updates:

- Mouse clicks on the panel are forwarded to `Panel::handle_mouse_press`.
- Key presses are forwarded to `Panel::handle_key_press` when the search bar is focused.
- Panel redraws trigger `render_frame()` so the panel remains consistent.

## Style and rendering behavior

- The panel is drawn as a rounded rectangle at the bottom of the desktop.
- The search bar is rendered with a consistent background color and a find icon.
- The search bar no longer uses hover-based sky-blue fill when focused.
- The panel now updates cleanly on typing without visible blinking caused by stale frame restoration.

## Build system updates

The Makefile now includes `ui/panel/src` files in `SRC_FILES` and adds `ui/panel/include` to `INCLUDE_DIRS`.

This ensures the new panel module is compiled and linked with the window manager.

## Notes

- The panel module is currently a static desktop UI subsystem and can be extended later with live task icons, notification areas, and interactive workspace elements.
- The modular design keeps panel rendering and input handling separate from the main window manager event loop.
