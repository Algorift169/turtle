#pragma once

#include <memory>
#include <string>

#include <X11/Xlib.h>

#include "background.hpp"
#include "cursor.hpp"

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
    std::unique_ptr<turtle::rc::RightClickController> rc_controller_;
    Pixmap background_pixmap_;
    int prev_cursor_x_;
    int prev_cursor_y_;
    int cursor_w_;
    int cursor_h_;
    bool background_pixmap_ready_;
};

}  // namespace wm
}  // namespace turtle
