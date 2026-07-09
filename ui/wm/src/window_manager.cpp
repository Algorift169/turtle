#include "../include/window_manager.hpp"

#include <iostream>
#include <string>

#include <X11/Xatom.h>
#include <X11/keysym.h>

namespace turtle {
namespace wm {

/*
 * window_manager.cpp
 * ------------------
 * Implements the `WindowManager` class which owns X11 resources and the main
 * event loop. Key responsibilities implemented here:
 * - create_desktop_window(): creates an InputOutput window that represents
 *   the desktop surface.
 * - initialize(): opens the X display, initializes the background renderer
 *   and the cursor renderer, captures a wallpaper snapshot to a pixmap for
 *   efficient region restoration, and primes the first frame.
 * - run(): event-driven main loop processing XEvents and routing them to
 *   handle_event().
 * - handle_event(): performs fine-grained handling for motion, button and
 *   expose events. Motion handling restores the previous cursor rectangle
 *   from `background_pixmap_` and draws the software cursor overlay.
 */

namespace {

void set_window_title(Display* display, Window window, const std::string& title) {
    XStoreName(display, window, title.c_str());
}

}  // namespace

WindowManager::WindowManager()
        : display_(nullptr), screen_(0), root_window_(0), desktop_window_(0), width_(0), height_(0), running_(false),
            background_pixmap_(0), prev_cursor_x_(0), prev_cursor_y_(0), cursor_w_(24), cursor_h_(24), background_pixmap_ready_(false) {}
            // Construct an empty WindowManager. Resources are acquired in `initialize()`.

WindowManager::~WindowManager() {
    cleanup();
}

bool WindowManager::initialize() {
    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr) {
        std::cerr << "WindowManager: unable to open X display." << std::endl;
        return false;
    }

    screen_ = DefaultScreen(display_);
        // Cache commonly used display/screen values.
    root_window_ = RootWindow(display_, screen_);
    width_ = DisplayWidth(display_, screen_);
    height_ = DisplayHeight(display_, screen_);

    if (!create_desktop_window()) {
        std::cerr << "WindowManager: failed to create the desktop window." << std::endl;
        cleanup();
        return false;
    }

    if (!cursor_.initialize(display_, desktop_window_)) {
            // Initialize the cursor renderer (software overlay only). The renderer
            // intentionally does not hide or modify the system cursor.
        std::cerr << "WindowManager: failed to install the custom cursor." << std::endl;
        cleanup();
        return false;
    }

    const std::string wallpaper_path = bckg::find_default_wallpaper_path();
    if (!background_.load_from_file(wallpaper_path)) {
        std::cerr << "WindowManager: unable to load wallpaper from " << wallpaper_path
                  << ". The expected file must exist before the window manager can render the desktop." << std::endl;
        cleanup();
        return false;
    }

        // Successfully initialized; prepare an initial wallpaper snapshot and
        // draw the first frame. After initialization the manager enters its
        // event-driven main loop in `run()`.
    running_ = true;

    // Render only the wallpaper into the desktop window and snapshot it into
    // a pixmap so the software cursor overlay can later restore small regions
    // from this snapshot. We avoid including the software cursor in the pixmap.
    background_.render_to_window(display_, desktop_window_, width_, height_);
    background_pixmap_ = XCreatePixmap(display_, desktop_window_, width_, height_,
                                       DefaultDepth(display_, screen_));
    GC tmp_gc = XCreateGC(display_, desktop_window_, 0, nullptr);
    XCopyArea(display_, desktop_window_, background_pixmap_, tmp_gc, 0, 0, width_, height_, 0, 0);
    XFreeGC(display_, tmp_gc);
    prev_cursor_x_ = 0;
    prev_cursor_y_ = 0;
    cursor_w_ = 24;
    cursor_h_ = 24;
    background_pixmap_ready_ = true;

    // Draw the software cursor on top of the desktop (not included in the pixmap).
    cursor_.draw_overlay();
    return true;
}

int WindowManager::run() {
    if (display_ == nullptr || desktop_window_ == 0) {
        return 1;
    }

    // The desktop should be redrawn only when the X server signals a reason for it.
    // Redrawing continuously causes visible flashing and wastes CPU while the WM is idle.
    while (running_) {
        XEvent event;
        XNextEvent(display_, &event);
        handle_event(event);
    }

    cleanup();
    return 0;
}

void WindowManager::shutdown() {
    running_ = false;
}

bool WindowManager::create_desktop_window() {
    XSetWindowAttributes attributes;
    attributes.background_pixel = BlackPixel(display_, screen_);
    attributes.override_redirect = False;

    desktop_window_ = XCreateWindow(display_, root_window_, 0, 0, width_, height_, 0,
                                    CopyFromParent, InputOutput, CopyFromParent,
                                    CWBackPixel | CWOverrideRedirect, &attributes);
    if (desktop_window_ == 0) {
        return false;
    }

    set_window_title(display_, desktop_window_, "Turtle Window Manager");

    Atom delete_window = XInternAtom(display_, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display_, desktop_window_, &delete_window, 1);

    XSelectInput(display_, desktop_window_, ExposureMask | KeyPressMask | StructureNotifyMask |
                                               SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XMapRaised(display_, desktop_window_);
    XFlush(display_);
    return true;
}

void WindowManager::render_frame() {
    XClearWindow(display_, desktop_window_);
    background_.render_to_window(display_, desktop_window_, width_, height_);
    cursor_.draw_overlay();
    XFlush(display_);
}

void draw_cursor_only() {
}

void WindowManager::handle_event(const XEvent& event) {
    switch (event.type) {
        case Expose:
            render_frame();
            break;

        case ConfigureNotify: {
            width_ = event.xconfigure.width;
            height_ = event.xconfigure.height;
            // Re-render wallpaper at the new size and recreate the background
            // pixmap so subsequent restores use the resized snapshot (don't
            // include the software cursor in the pixmap).
            background_.render_to_window(display_, desktop_window_, width_, height_);
            if (background_pixmap_ != 0) {
                XFreePixmap(display_, background_pixmap_);
                background_pixmap_ = 0;
            }
            background_pixmap_ = XCreatePixmap(display_, desktop_window_, width_, height_,
                                               DefaultDepth(display_, screen_));
            GC tmp_gc = XCreateGC(display_, desktop_window_, 0, nullptr);
            XCopyArea(display_, desktop_window_, background_pixmap_, tmp_gc, 0, 0, width_, height_, 0, 0);
            XFreeGC(display_, tmp_gc);
            background_pixmap_ready_ = true;
            cursor_.draw_overlay();
            break;
        }

        case KeyPress: {
            KeySym key_symbol = XLookupKeysym(const_cast<XKeyEvent*>(&event.xkey), 0);
            if (key_symbol == XK_Escape) {
                shutdown();
            }
            break;
        }

        case ClientMessage: {
            Atom delete_window = XInternAtom(display_, "WM_DELETE_WINDOW", False);
            if (event.xclient.data.l[0] == static_cast<long>(delete_window)) {
                shutdown();
            }
            break;
        }

        case MotionNotify: {
            int nx = event.xmotion.x;
            int ny = event.xmotion.y;
            if (background_pixmap_ready_) {
                GC gc = XCreateGC(display_, desktop_window_, 0, nullptr);
                XCopyArea(display_, background_pixmap_, desktop_window_, gc,
                          prev_cursor_x_, prev_cursor_y_, cursor_w_, cursor_h_, prev_cursor_x_, prev_cursor_y_);
                XFreeGC(display_, gc);
            }
            cursor_.set_position(nx, ny);
            cursor_.draw_overlay();
            XFlush(display_);
            prev_cursor_x_ = nx;
            prev_cursor_y_ = ny;
            break;
        }

        case ButtonPress: {
            // Restore previous cursor region before drawing click indicator
            if (background_pixmap_ready_) {
                GC gc = XCreateGC(display_, desktop_window_, 0, nullptr);
                XCopyArea(display_, background_pixmap_, desktop_window_, gc,
                          prev_cursor_x_, prev_cursor_y_, cursor_w_, cursor_h_, prev_cursor_x_, prev_cursor_y_);
                XFreeGC(display_, gc);
            }
            if (event.xbutton.button == Button1) {
                cursor_.set_state(cursor::CursorState::LeftClick);
            } else if (event.xbutton.button == Button3) {
                cursor_.set_state(cursor::CursorState::RightClick);
            } else if (event.xbutton.button == Button4) {
                cursor_.set_state(cursor::CursorState::ScrollUp);
            } else if (event.xbutton.button == Button5) {
                cursor_.set_state(cursor::CursorState::ScrollDown);
            }
            cursor_.draw_overlay();
            XFlush(display_);
            break;
        }

        case ButtonRelease:
            if (background_pixmap_ready_) {
                GC gc = XCreateGC(display_, desktop_window_, 0, nullptr);
                XCopyArea(display_, background_pixmap_, desktop_window_, gc,
                          prev_cursor_x_, prev_cursor_y_, cursor_w_, cursor_h_, prev_cursor_x_, prev_cursor_y_);
                XFreeGC(display_, gc);
            }
            cursor_.set_state(cursor::CursorState::Normal);
            cursor_.draw_overlay();
            XFlush(display_);
            break;

        case DestroyNotify:
            shutdown();
            break;

        default:
            break;
    }
}

void WindowManager::cleanup() {
    if (display_ != nullptr) {
        if (desktop_window_ != 0) {
            XDestroyWindow(display_, desktop_window_);
            desktop_window_ = 0;
        }
        if (background_pixmap_ != 0) {
            XFreePixmap(display_, background_pixmap_);
            background_pixmap_ = 0;
        }
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

}  // namespace wm
}  // namespace turtle
