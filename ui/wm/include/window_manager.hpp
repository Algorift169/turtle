#pragma once

#include <memory>
#include <string>

#include <X11/Xlib.h>

#include "background.hpp"
#include "cursor.hpp"
#include "../../panel/include/panel.hpp"

namespace turtle { namespace rc { class RightClickController; } }

/*
 * WindowManager
 * ----------------
 * Core responsibilities:
 * - Open and manage the X11 `Display` and create a full-screen desktop window.
 * - Integrate the `BackgroundRenderer` to paint the wallpaper into the desktop
 *   window and keep a pixmap snapshot for small-region restores.
 * - Integrate the `CursorRenderer` to draw a software cursor overlay without
 *   touching the system cursor.
 * - Run the main event loop and dispatch X11 events (Expose, ConfigureNotify,
 *   MotionNotify, ButtonPress/Release, KeyPress, ClientMessage).
 *
 * Implementation notes:
 * - A `background_pixmap_` contains a snapshot of the wallpaper-only content
 *   so the manager can restore only the area previously occupied by a
 *   software-drawn cursor (avoiding full-window repaints on motion).
 */

namespace turtle {
namespace wm {

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    bool initialize();
    int run();
    void shutdown();

    // Drag API for desktop icons
    // Usage:
    //  - call `startLauncherDrag(pointer_x, pointer_y)` when a press begins
    //  - call `updateLauncherDrag(pointer_x, pointer_y)` while pointer moves
    //  - call `finishLauncherDrag(pointer_x, pointer_y)` on release; it returns
    //    true if the launcher should be activated (click without movement)
    void startLauncherDrag(int pointer_x, int pointer_y);
    void updateLauncherDrag(int pointer_x, int pointer_y);
    bool finishLauncherDrag(int pointer_x, int pointer_y);
    // Return true if the given desktop coordinates are over any desktop icon.
    // Currently checks the file-manager launcher; expand this to include
    // additional desktop icons as needed.
    bool isPointOverDesktopIcon(int x, int y) const;
    

private:
    bool create_desktop_window();
    void render_frame();
    void handle_event(const XEvent& event);
    void cleanup();

    Display* display_;
    int screen_;
    Window root_window_;
    Window desktop_window_;
    int width_;
    int height_;
    bool running_;
    bckg::BackgroundRenderer background_;
    cursor::CursorRenderer cursor_;
    panel::Panel panel_;
    std::unique_ptr<turtle::rc::RightClickController> rc_controller_;
    Pixmap background_pixmap_;
    Pixmap frame_pixmap_;
    int prev_cursor_x_;
    int prev_cursor_y_;
    int cursor_w_;
    int cursor_h_;
    bool background_pixmap_ready_;
    bool frame_pixmap_ready_;
    // Desktop launcher icon state
    int launcher_x_ = 24;
    int launcher_y_ = 64;
    int launcher_w_ = 88;
    int launcher_h_ = 88;
    bool launcher_dragging_ = false;
    int launcher_drag_offset_x_ = 0;
    int launcher_drag_offset_y_ = 0;
    int launcher_initial_x_ = 24;
    int launcher_initial_y_ = 64;
    bool launcher_moved_ = false;
};

}  // namespace wm
}  // namespace turtle
