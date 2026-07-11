#include "../include/window_manager.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <Imlib2.h>
#include "../../../controls/rc/include/rc1.hpp"

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

bool point_in_rect(int px, int py, int x, int y, int width, int height) {
    return px >= x && px < x + width && py >= y && py < y + height;
}

bool launch_file_manager() {
    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::current_path() / "build" / "bin" / "turtle-file-manager",
        std::filesystem::path("build/bin/turtle-file-manager"),
        std::filesystem::path("/home/israfil/Desktop/turtle/build/bin/turtle-file-manager")
    };

    for (const auto& candidate : candidates) {
        if (!std::filesystem::exists(candidate) || !std::filesystem::is_regular_file(candidate)) {
            continue;
        }

        const pid_t pid = fork();
        if (pid == 0) {
            const std::string executable = candidate.string();
            char* const argv[] = {const_cast<char*>(executable.c_str()), nullptr};
            execv(executable.c_str(), argv);
            _exit(127);
        }
        return pid >= 0;
    }

    return false;
}

void draw_icon(Display* display, Window window, const std::string& icon_path, int x, int y, int size) {
    if (icon_path.empty() || !std::filesystem::exists(icon_path)) {
        return;
    }

    Imlib_Image image = imlib_load_image(icon_path.c_str());
    if (!image) {
        return;
    }

    imlib_context_set_display(display);
    imlib_context_set_visual(DefaultVisual(display, DefaultScreen(display)));
    imlib_context_set_colormap(DefaultColormap(display, DefaultScreen(display)));
    imlib_context_set_drawable(window);
    imlib_context_set_image(image);
    imlib_render_image_on_drawable_at_size(x, y, size, size);
    imlib_free_image();
}

void draw_file_manager_launcher(Display* display, Window window, int desktop_width, int desktop_height) {
    const int button_x = 24;
    const int button_y = 64;
    const int button_w = 88;
    const int button_h = 88;
    const int icon_size = 48;

    if (desktop_width <= button_x + button_w || desktop_height <= button_y + button_h) {
        return;
    }

    GC gc = XCreateGC(display, window, 0, nullptr);
    XSetForeground(display, gc, 0x22000000);
    XFillRectangle(display, window, gc, button_x, button_y, button_w, button_h);
    XSetForeground(display, gc, 0x66000000);
    XDrawRectangle(display, window, gc, button_x, button_y, button_w, button_h);

    const std::string label = "File-system";
    // Try to load a bold font; fall back to fixed if not available.
    XFontStruct* font = XLoadQueryFont(display, "-*-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*");
    if (!font) font = XLoadQueryFont(display, "fixed");
    if (font) {
        XSetFont(display, gc, font->fid);
        int text_width = XTextWidth(font, label.c_str(), static_cast<int>(label.size()));
        int text_x = button_x + (button_w - text_width) / 2;
        int text_y = button_y + button_h - 12;
        XSetForeground(display, gc, 0xFFFFFFFF);
        XDrawString(display, window, gc, text_x, text_y, label.c_str(), static_cast<int>(label.size()));
        XFreeFont(display, font);
    }
    XFreeGC(display, gc);

    const std::vector<std::filesystem::path> icon_candidates = {
        std::filesystem::current_path() / "images" / "icons" / "file-manager.svg",
        std::filesystem::path("images/icons/file-manager.svg"),
        std::filesystem::path("/home/israfil/Desktop/turtle/images/icons/file-manager.svg")
    };

    for (const auto& candidate : icon_candidates) {
        if (std::filesystem::exists(candidate)) {
            draw_icon(display, window, candidate.string(), button_x + 20, button_y + 8, icon_size);
            break;
        }
    }
}

}  // namespace

WindowManager::WindowManager()
        : display_(nullptr), screen_(0), root_window_(0), desktop_window_(0), width_(0), height_(0), running_(false),
            background_pixmap_(0), frame_pixmap_(0), prev_cursor_x_(0), prev_cursor_y_(0), cursor_w_(24), cursor_h_(24),
            background_pixmap_ready_(false), frame_pixmap_ready_(false) {}
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

    if (!panel_.load_style()) {
        std::cerr << "WindowManager: failed to load panel style." << std::endl;
        cleanup();
        return false;
    }

    rc_controller_ = std::make_unique<turtle::rc::RightClickController>();

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

    // Render the wallpaper and panel into the desktop window and snapshot it
    // into a pixmap so the software cursor overlay can later restore small
    // regions from a panel-aware frame instead of wiping the panel away.
    background_.render_to_window(display_, desktop_window_, width_, height_);
    panel_.draw(display_, desktop_window_, width_, height_, prev_cursor_x_, prev_cursor_y_);
    background_pixmap_ = XCreatePixmap(display_, desktop_window_, width_, height_,
                                       DefaultDepth(display_, screen_));
    frame_pixmap_ = XCreatePixmap(display_, desktop_window_, width_, height_,
                                  DefaultDepth(display_, screen_));
    GC tmp_gc = XCreateGC(display_, desktop_window_, 0, nullptr);
    XCopyArea(display_, desktop_window_, background_pixmap_, tmp_gc, 0, 0, width_, height_, 0, 0);
    XCopyArea(display_, desktop_window_, frame_pixmap_, tmp_gc, 0, 0, width_, height_, 0, 0);
    XFreeGC(display_, tmp_gc);
    prev_cursor_x_ = 0;
    prev_cursor_y_ = 0;
    cursor_w_ = 24;
    cursor_h_ = 24;
    background_pixmap_ready_ = true;
    frame_pixmap_ready_ = true;

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
    // Double-buffer the entire frame: compose into frame_pixmap_ first, then
    // present to the window in a single blit to avoid visible flicker (e.g.
    // the search-bar blink caused by momentarily showing stale content).
    if (frame_pixmap_ == 0) {
        frame_pixmap_ = XCreatePixmap(display_, desktop_window_, width_, height_,
                                      DefaultDepth(display_, screen_));
    }

    // Step 1: draw the wallpaper into the off-screen pixmap.
    if (background_pixmap_ready_) {
        GC gc = XCreateGC(display_, desktop_window_, 0, nullptr);
        XCopyArea(display_, background_pixmap_, frame_pixmap_, gc,
                  0, 0, width_, height_, 0, 0);
        XFreeGC(display_, gc);
    } else {
        background_.render_to_window(display_, frame_pixmap_, width_, height_);
    }

    // Step 2: draw the file-manager launcher button onto the desktop surface.
    draw_file_manager_launcher(display_, frame_pixmap_, width_, height_);

    // Step 3: draw the panel into the off-screen pixmap.
    panel_.draw(display_, frame_pixmap_, width_, height_, prev_cursor_x_, prev_cursor_y_);

    // Step 4: draw the right-click menu (if any) into the off-screen pixmap.
    if (rc_controller_ && rc_controller_->is_open()) {
        rc_controller_->draw(display_, frame_pixmap_);
    }

    // Step 5: present the completed frame to the window in one
    // operation, then draw the cursor overlay on top.
    {
        GC gc = XCreateGC(display_, desktop_window_, 0, nullptr);
        XCopyArea(display_, frame_pixmap_, desktop_window_, gc,
                  0, 0, width_, height_, 0, 0);
        XFreeGC(display_, gc);
    }

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
            panel_.draw(display_, desktop_window_, width_, height_, prev_cursor_x_, prev_cursor_y_);
            if (background_pixmap_ != 0) {
                XFreePixmap(display_, background_pixmap_);
                background_pixmap_ = 0;
            }
            if (frame_pixmap_ != 0) {
                XFreePixmap(display_, frame_pixmap_);
                frame_pixmap_ = 0;
            }
            background_pixmap_ = XCreatePixmap(display_, desktop_window_, width_, height_,
                                               DefaultDepth(display_, screen_));
            frame_pixmap_ = XCreatePixmap(display_, desktop_window_, width_, height_,
                                          DefaultDepth(display_, screen_));
            GC tmp_gc = XCreateGC(display_, desktop_window_, 0, nullptr);
            XCopyArea(display_, desktop_window_, background_pixmap_, tmp_gc, 0, 0, width_, height_, 0, 0);
            XCopyArea(display_, desktop_window_, frame_pixmap_, tmp_gc, 0, 0, width_, height_, 0, 0);
            XFreeGC(display_, tmp_gc);
            background_pixmap_ready_ = true;
            frame_pixmap_ready_ = true;
            cursor_.draw_overlay();
            break;
        }

        case KeyPress: {
            if (panel_.handle_key_press(event.xkey)) {
                render_frame();
                break;
            }
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

            if (rc_controller_ && rc_controller_->is_open()) {
                if (frame_pixmap_ready_) {
                    GC gc = XCreateGC(display_, desktop_window_, 0, nullptr);
                    XCopyArea(display_, frame_pixmap_, desktop_window_, gc,
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

            if (frame_pixmap_ready_) {
                GC gc = XCreateGC(display_, desktop_window_, 0, nullptr);
                XCopyArea(display_, frame_pixmap_, desktop_window_, gc,
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
            if (event.xbutton.button == Button3) {
                if (rc_controller_) {
                    rc_controller_->on_right_click(event.xbutton.x, event.xbutton.y);
                }
                render_frame();
                break;
            }

            if (rc_controller_ && rc_controller_->is_open()) {
                rc_controller_->on_click(event.xbutton.x, event.xbutton.y);
                render_frame();
                break;
            }

            const int launcher_x = 24;
            const int launcher_y = 24;
            const int launcher_w = 88;
            const int launcher_h = 88;
            if (event.xbutton.button == Button1 && point_in_rect(event.xbutton.x, event.xbutton.y, launcher_x, launcher_y, launcher_w, launcher_h)) {
                launch_file_manager();
                render_frame();
                break;
            }

            if (panel_.handle_mouse_press(event.xbutton.x, event.xbutton.y, width_, height_)) {
                render_frame();
                break;
            }

            // Restore previous cursor region before drawing click indicator
            if (frame_pixmap_ready_) {
                GC gc = XCreateGC(display_, desktop_window_, 0, nullptr);
                XCopyArea(display_, frame_pixmap_, desktop_window_, gc,
                          prev_cursor_x_, prev_cursor_y_, cursor_w_, cursor_h_, prev_cursor_x_, prev_cursor_y_);
                XFreeGC(display_, gc);
            }
            if (event.xbutton.button == Button1) {
                cursor_.set_state(cursor::CursorState::LeftClick);
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
            if (frame_pixmap_ready_) {
                GC gc = XCreateGC(display_, desktop_window_, 0, nullptr);
                XCopyArea(display_, frame_pixmap_, desktop_window_, gc,
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
        if (frame_pixmap_ != 0) {
            XFreePixmap(display_, frame_pixmap_);
            frame_pixmap_ = 0;
        }
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

}  // namespace wm
}  // namespace turtle
