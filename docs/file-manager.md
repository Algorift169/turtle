# Turtle File Manager

## Overview

`tools/file-manager` is Turtle's first standalone desktop application. It is a GTK 3 executable named `turtle-file-manager`; it deliberately runs outside the X11 window-manager process. That split makes the application usable from a future launcher, desktop entry, or IPC service without making GTK part of the window manager's lifecycle.

## Module responsibilities

| Component | Responsibility |
| --- | --- |
| `FileSystem` | Reads real Linux directories, classifies entries, and reports filesystem errors without UI dependencies. |
| `NavigationController` | Owns current location and back/forward history; emits location changes to the window. |
| `IconLoader` | Prefers Turtle assets in `images/icons` and supplies GTK fallbacks if an optional asset is missing. |
| `Sidebar` | Provides common locations and emits navigation requests. |
| `Toolbar` | Provides navigation controls, the read-only current-location field, and the UI-only search field. |
| `DirectoryView` | Displays selectable filesystem entries and reports activation/selection events. |
| `PreviewPanel` | Selects the current lightweight preview: image, text, directory, video placeholder, or generic file information. |
| `FileManagerWindow` | Composition root that connects GTK widgets to navigation and filesystem services. |

## Navigation and directory display

Navigation begins in the user's home directory. Sidebar locations and folder activation request a location through `FileManagerWindow`; the window validates it with `FileSystem` before updating `NavigationController`. The controller owns history, then invokes one location-change callback. That callback refreshes the path bar, toolbar sensitivity, listing, and preview state together.

Directories are enumerated with `std::filesystem` and `skip_permission_denied`. Entries are sorted with directories first. Invalid paths, unreadable locations, and iterator errors are turned into visible messages instead of exceptions escaping into GTK callbacks.

## Preview system

The initial preview system is intentionally lightweight. Images use GDK-Pixbuf scaling, text files display a short read-only excerpt, directories display navigation information, videos identify themselves as awaiting a media provider, and other files show generic metadata. `PreviewPanel` is the extension boundary for richer thumbnail, MIME, video, and document preview providers.

## Window chrome, toolbar, and menus

The GTK header bar displays `Turtle File-System` and custom circular yellow, green, and red controls for minimize, fullscreen toggle, and close. The toolbar uses Turtle navigation assets for Back, Forward, and Up. Its path field is read-only by design today, so it represents the current location rather than pretending to be a search box; it can become an editable location entry without changing navigation ownership. The search entry accepts text but deliberately has no backend yet.

The File, Edit, View, Bookmarks, Recent, and Help menus contain placeholder actions. They are separate GTK menus so application actions can be introduced incrementally.

## Desktop integration

The window manager draws `images/icons/file-manager.svg` on the desktop and owns only hit testing and launching. A double click forks and executes the adjacent `turtle-file-manager` binary. This keeps desktop launching independent from the File Manager's GTK code and follows normal process boundaries.

## Build and roadmap

The top-level Makefile now builds both `build/bin/turtle-wm` and `build/bin/turtle-file-manager`. GTK and GDK-Pixbuf flags are applied only to the File Manager target; existing X11/Imlib2 window-manager linking remains intact.

Next steps include editable path navigation, MIME-aware icon resolution, actual search, richer preview providers, file operations, persistent bookmarks and recents, desktop-entry installation, and a future application-launch service shared by desktop icons and panel launchers.

## Known limitations

- Search and menu actions are intentionally UI-only.
- Text previews are limited to a small plain-text excerpt.
- Video previews are placeholders; no media engine is linked.
- `Recent` currently opens Home until a recents service exists.
- The desktop launcher expects the File Manager binary alongside `turtle-wm` in `build/bin`.
