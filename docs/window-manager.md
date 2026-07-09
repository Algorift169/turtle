# Turtle Window Manager

## What was implemented

This first pass introduces a minimal desktop environment foundation for Turtle.
It provides:

- a dedicated window manager entry point under [ui/wm](ui/wm)
- a background renderer under [ui/bckg](ui/bckg)
- a desktop window that fills the display area
- a basic event loop for expose, resize, key, and close events
- a small build system and Xephyr launch script

## Directory structure

- [ui/wm/include](ui/wm/include) holds the window manager public interface.
- [ui/wm/src](ui/wm/src) contains the implementation and the main entry point.
- [ui/bckg/include](ui/bckg/include) holds the background renderer interface.
- [ui/bckg/src](ui/bckg/src) contains the wallpaper-loading and drawing implementation.
- [docs](docs) stores developer documentation for the subsystem.

## Responsibilities of each module

### Window manager

The window manager owns the application's lifecycle. It opens an X11 display, creates a full-screen desktop window, renders the wallpaper, and processes the main event loop.

### Background renderer

The background renderer is intentionally independent from the window manager. It owns image loading and rendering responsibilities so future wallpaper switching can happen through this module without changing the WM code path.

## How the window manager starts

The executable is built as [build/bin/turtle-wm](build/bin/turtle-wm). The program starts in [ui/wm/src/main.cpp](ui/wm/src/main.cpp), constructs a window manager instance, initializes X11 resources, and enters the event loop.

## How the background renderer works

The background renderer loads the default wallpaper from [images/logo/main/main.png](images/logo/main/main.png) when available. It uses Imlib2 to decode the image and draw it into the desktop window. The renderer is designed so a future wallpaper-switching API can be added without touching the window manager.

## How the event loop operates

The loop in [ui/wm/src/window_manager.cpp](ui/wm/src/window_manager.cpp) repeatedly checks for X11 events. It handles:

- expose events to redraw the desktop
- configure events to track resize changes
- key presses, including Escape for shutdown
- close requests from the window manager protocol

## Future extension points

The current implementation leaves room for:

- multiple managed windows
- mouse interaction
- keyboard shortcuts
- a compositor layer
- workspaces
- panels and notifications

These features can be layered on top of the same event loop and background module without needing to redesign the current architecture.

## Known limitations

- Only one desktop window is created.
- No window decorations, stacking, or focus management exist yet.
- There is no compositor or input device abstraction yet.
- Wallpaper selection is currently limited to a single default asset path.

## Design decisions

- The background subsystem is isolated from the window manager so wallpaper handling can evolve independently.
- Rendering and initialization are separated to keep resource loading and frame drawing distinct.
- The current architecture favors a simple, explicit event loop rather than premature abstractions.

## Build instructions

Run the following from the repository root:

```bash
make
```

The binary is emitted to [build/bin/turtle-wm](build/bin/turtle-wm).

To launch it under Xephyr:

```bash
./script/run.sh
```

## Developer Notes

- **Architecture:** The project separates responsibilities into small, well-scoped modules: the window manager (`ui/wm`) controls X11 interaction and application lifecycle; the background renderer (`ui/bckg`) is responsible for image loading and rendering; the cursor renderer (`ui/cursor`) implements a software overlay cursor drawn on top of the desktop window without modifying the system cursor.
- **Software cursor design:** The software cursor is intentionally implemented as an overlay drawn into the desktop window. To avoid leaving visual artifacts, the WM maintains a background pixmap snapshot and restores only the rectangle previously occupied by the cursor before drawing the cursor at a new position.
- **Performance:** The WM is event-driven (no busy redraw loop). Full desktop redraws happen only on `Expose` or resize events. Cursor motion uses targeted region blits (`XCopyArea`) to minimize repaint cost.
- **Extensibility:** Modules are designed to be replaced or extended independently. For example, a compositor can replace the simple `render_to_window` call in `BackgroundRenderer` without touching the WM logic.

If you want broader inline documentation or a code tour, tell me which modules you'd like expanded first (WM, cursor, background, or build). I can also generate an API-style README for each module.
