Turtle OS
=====================================

Overview
--------
Turtle is a small, experimental desktop/window-manager foundation using X11 and Imlib2.
It provides:
- a full-screen desktop window (WM skeleton)
- a background renderer (Imlib2)
- a software-drawn cursor overlay (does NOT modify the system cursor)
- a bottom panel UI with a search bar and quick-launch buttons

Quick build
-----------
Install required packages (Debian/Ubuntu):

```bash
sudo apt install build-essential pkg-config libx11-dev libimlib2-dev x11-utils x11-xserver-utils
```

Build and run:

```bash
make clean && make
./script/run.sh   # launches under Xephyr (see script/run.sh)
```

Project layout
--------------
- `ui/wm/` — Window manager: `include` and `src` (main, event loop)
- `ui/bckg/` — Background renderer (Imlib2)
- `ui/cursor/` — Software cursor overlay renderer
- `ui/panel/` — Bottom panel taskbar UI and search input
- `style/cursor/` — Cursor style definitions
- `style/panel/` — Panel styling definitions
- `ui/tabs/` — Right-click desktop menu implementation
- `images/` — Default wallpaper and icon assets
- `script/run.sh` — Helper to run under Xephyr

Developer notes
---------------
- The software cursor draws into the desktop window. To avoid artifacts the
  WM keeps a `background_pixmap` snapshot and restores the previous cursor
  rectangle with `XCopyArea` before drawing the cursor at the new position.
- The code intentionally does **not** hide or redefine the system cursor.
- Modules are small and isolated so you can replace the renderer or add a
  compositor without large refactors.
- The new panel subsystem is integrated into the window manager and supports
  panel rendering, search input, and click handling.

VS Code include troubleshooting
------------------------------
If your editor flags missing headers, add these include paths in
`.vscode/c_cpp_properties.json` or your C/C++ extension settings:

- `${workspaceFolder}/ui/wm/include`
- `${workspaceFolder}/ui/bckg/include`
- `${workspaceFolder}/ui/cursor/include`
- `${workspaceFolder}/ui/panel/include`
- `${workspaceFolder}/session/include`

Example `c_cpp_properties.json` entry (Linux):

```json
{
  "configurations": [
    {
      "name": "Linux",
      "includePath": [
        "${workspaceFolder}/ui/wm/include",
        "${workspaceFolder}/ui/bckg/include",
        "${workspaceFolder}/ui/cursor/include",
        "${workspaceFolder}/ui/panel/include",
        "${workspaceFolder}/**"
      ]
    }
  ]
}
```

Next steps
----------
- `make` then run `./script/run.sh` in Xephyr to test the desktop, panel, and cursor.
- See `docs/ui-interaction.md` for the right-click menu and cursor behavior.
- See `docs/panel.md` for the new panel implementation and search input details.

Session Manager
---------------
The Session Manager orchestrates Turtle's runtime services (Window Manager,
Background Manager, etc.). See the developer documentation for details:

- [Session Manager design and usage](docs/session-manager.md)
- [UI interaction and cursor style](docs/ui-interaction.md)
- [Panel subsystem and search input](docs/panel.md)
