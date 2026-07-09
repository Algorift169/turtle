Turtle OS
=====================================

Overview
--------
Turtle is a small, experimental desktop/window-manager foundation using X11 and Imlib2.
It provides:
- a full-screen desktop window (WM skeleton)
- a background renderer (Imlib2)
- a software-drawn cursor overlay (does NOT modify the system cursor)

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
- `images/` — Default wallpaper: `images/logo/main/main.png`
- `script/run.sh` — Helper to run under Xephyr

Developer notes
---------------
- The software cursor draws into the desktop window. To avoid artifacts the
  WM keeps a `background_pixmap` snapshot and restores the previous cursor
  rectangle with `XCopyArea` before drawing the cursor at the new position.
- The code intentionally does **not** hide or redefine the system cursor.
- Modules are small and isolated so you can replace the renderer or add a
  compositor without large refactors.

VS Code include troubleshooting
------------------------------
If your editor flags missing headers, add these include paths in
`.vscode/c_cpp_properties.json` or your C/C++ extension settings:

- `${workspaceFolder}/ui/wm/include`
- `${workspaceFolder}/ui/bckg/include`
- `${workspaceFolder}/ui/cursor/include`
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
        "${workspaceFolder}/**"
      ]
    }
  ]
}
```

Next steps
----------
- `make` then run `./script/run.sh` in Xephyr to test the desktop and cursor.
- Tell me if you want more detailed module-level READMEs or Doxygen-style
  comments added to specific files.

Session Manager
---------------
The Session Manager orchestrates Turtle's runtime services (Window Manager,
Background Manager, etc.). See the developer documentation for details:

- [Session Manager design and usage](docs/session-manager.md)
# turtle
