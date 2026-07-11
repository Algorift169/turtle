Implementation Notes: panel & window-manager changes

Files covered:
- ui/panel/include/panel.hpp
- ui/panel/src/panel.cpp
- ui/wm/include/window_manager.hpp
- ui/wm/src/window_manager.cpp

Summary
- These changes implement a top-positioned panel, adjust icon/button sizing, add a desktop launcher icon with drag support, and improve event handling to prevent desktop icon interactions from triggering the `w-tab` UI.

Details

1) ui/panel/include/panel.hpp
- Added `int height() const` to expose the panel's current height to external components (e.g., the window manager). This allows the WM to constrain desktop icon placement and prevent overlap with the panel.

2) ui/panel/src/panel.cpp
- Implemented `height()` to return the panel's configured or computed height.
- Adjusted panel drawing logic to place the panel at the top of the screen instead of the bottom, and reduced the search bar height while increasing the size of several buttons (turtle, calculator, browser, calendar) for improved ergonomics.
- `handle_mouse_press` now correctly dispatches events to panel elements and returns a boolean to indicate whether the click was consumed.

3) ui/wm/include/window_manager.hpp
- Added a drag API for desktop icons: `startLauncherDrag`, `updateLauncherDrag`, `finishLauncherDrag`.
- Declared `isPointOverDesktopIcon(int x, int y) const` to let other components query whether a pointer location lies over a desktop icon; this is used to decide whether to consume pointer events (notably right-clicks).
- Added internal state variables to store launcher position/size and drag state: `launcher_x_`, `launcher_y_`, `launcher_w_`, `launcher_h_`, `launcher_dragging_`, `launcher_drag_offset_x_`, `launcher_drag_offset_y_`, `launcher_initial_x_`, `launcher_initial_y_`, `launcher_moved_`.

4) ui/wm/src/window_manager.cpp
- Implemented the launcher drag API:
  - `startLauncherDrag(pointer_x, pointer_y)` captures an initial offset and sets `launcher_dragging_ = true`.
  - `updateLauncherDrag(pointer_x, pointer_y)` computes a constrained new position, restores the previous background area (using the `background_pixmap_` snapshot when available) to avoid full repaints, draws the launcher at its new position, and tracks whether the launcher moved enough to be considered a drag.
  - `finishLauncherDrag(pointer_x, pointer_y)` finalizes the drag and returns `true` if the launcher should be activated (click without movement). It also re-renders the frame and triggers `launch_file_manager()` when appropriate.
- Implemented `isPointOverDesktopIcon(int x, int y) const` to check the launcher rectangle. The function includes detailed comments explaining how to use and extend it: call it when handling `ButtonPress`/`ButtonRelease`/`MotionNotify` to decide whether to consume events, and expand it to check new icons if more are added.
- Modified `handle_event` `ButtonPress` handling to consume right-clicks (`Button3`) that occur over desktop icons. When `isPointOverDesktopIcon(...)` returns true, the WM now renders the frame and breaks without forwarding the event to `rc_controller_` or other UI handlers, preventing `w-tab` from opening for desktop icon right-clicks.
- Removed a filled rectangle border previously drawn behind the launcher to avoid a black box artifact; the launcher is now drawn over the wallpaper with alpha as needed.
- Optimized repaint behavior during dragging to limit redraw area and reduce flicker.

Notes & Extension Points
- `isPointOverDesktopIcon` currently checks only the file-manager launcher. For multiple icons, maintain a container (vector) of icon bounds and iterate to check hits. Keep the check O(n) with small n or consider a spatial index if many icons are added.
- The WM consumes right-clicks over desktop icons to prevent `w-tab` activation. If an icon should present its own context menu, implement an icon-specific context handler and call it when the WM consumes the event.
- The `finishLauncherDrag` logic treats small pointer movement as a click (activates launcher). The movement threshold is 4 pixels squared in Euclidean terms; adjust if required for different sensitivity.

Build & Test
- After these edits, run:

```bash
make -j4
```

- Manually test:
  - Right-click on the file-manager launcher: the `w-tab` UI should not open.
  - Left-click and drag the launcher: it should move smoothly and not flicker.
  - Click the launcher without moving it: file manager should launch.
  - Ensure panel remains at top and icons render below it.

Contact
- For questions about extending `isPointOverDesktopIcon` or adding additional desktop icons, review `ui/wm/include/window_manager.hpp` and `ui/wm/src/window_manager.cpp` where the drag API and state are implemented.
