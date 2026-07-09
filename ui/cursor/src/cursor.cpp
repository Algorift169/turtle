#include "../include/cursor.hpp"

#include <X11/Xutil.h>
#include <iostream>

namespace turtle {
namespace cursor {

// Helper drawing primitives
// -------------------------
// These small helper functions operate on a provided `GC` and draw simple
// vector shapes into the provided drawable. They are intentionally simple so
// the software cursor can be drawn with minimal overhead.
void draw_pointer(Display* display, Window window, GC gc, int x, int y) {
    XPoint points[7] = {
        {static_cast<short>(x), static_cast<short>(y)},
        {static_cast<short>(x + 2), static_cast<short>(y + 14)},
        {static_cast<short>(x + 5), static_cast<short>(y + 12)},
        {static_cast<short>(x + 10), static_cast<short>(y + 20)},
        {static_cast<short>(x + 8), static_cast<short>(y + 20)},
        {static_cast<short>(x + 6), static_cast<short>(y + 13)},
        {static_cast<short>(x + 3), static_cast<short>(y + 15)}
    };
    XFillPolygon(display, window, gc, points, 7, Convex, CoordModeOrigin);
}

void draw_left_click_indicator(Display* display, Window window, GC gc, int x, int y) {
    XFillRectangle(display, window, gc, x + 12, y + 4, 5, 5);
}

void draw_right_click_indicator(Display* display, Window window, GC gc, int x, int y) {
    XFillRectangle(display, window, gc, x + 12, y + 11, 5, 5);
}

void draw_scroll_up_indicator(Display* display, Window window, GC gc, int x, int y) {
    XPoint arrow[3] = {
        {static_cast<short>(x + 10), static_cast<short>(y + 1)},
        {static_cast<short>(x + 7), static_cast<short>(y + 5)},
        {static_cast<short>(x + 13), static_cast<short>(y + 5)}
    };
    XFillPolygon(display, window, gc, arrow, 3, Convex, CoordModeOrigin);
}

void draw_scroll_down_indicator(Display* display, Window window, GC gc, int x, int y) {
    XPoint arrow[3] = {
        {static_cast<short>(x + 10), static_cast<short>(y + 20)},
        {static_cast<short>(x + 7), static_cast<short>(y + 16)},
        {static_cast<short>(x + 13), static_cast<short>(y + 16)}
    };
    XFillPolygon(display, window, gc, arrow, 3, Convex, CoordModeOrigin);
}


// CursorRenderer implementation
// -----------------------------
// The renderer maintains a simple position and state and draws the
// overlay on demand. It does not manage or alter the system cursor.
CursorRenderer::CursorRenderer()
    : display_(nullptr), target_window_(0), root_window_(0), cursor_(0), current_state_(CursorState::Normal), x_(32), y_(32) {}

CursorRenderer::~CursorRenderer() {
    destroy_cursor();
}

void CursorRenderer::destroy_cursor() {
    // No system cursor resources are managed by this renderer any more.
}
bool CursorRenderer::initialize(Display* display, Window window) {
    if (display == nullptr || window == 0) {
        return false;
    }

    display_ = display;
    target_window_ = window;
    root_window_ = RootWindow(display_, DefaultScreen(display_));
    current_state_ = CursorState::Normal;
    // We purposely do NOT touch or change the system cursor. This renderer
    // draws a software cursor overlay on the desktop window only. The WM is
    // responsible for keeping an independent background snapshot used to
    // restore the underlying pixels before each overlay draw.
    return true;
}

void CursorRenderer::set_state(CursorState state) {
    current_state_ = state;
}

void CursorRenderer::set_position(int x, int y) {
    x_ = x;
    y_ = y;
}

void CursorRenderer::draw_overlay() const {
    if (display_ == nullptr || target_window_ == 0) {
        return;
    }

    XGCValues values;
    GC gc = XCreateGC(display_, target_window_, 0, &values);
    XSetForeground(display_, gc, WhitePixel(display_, DefaultScreen(display_)));
    XSetBackground(display_, gc, BlackPixel(display_, DefaultScreen(display_)));
    XSetLineAttributes(display_, gc, 2, LineSolid, CapButt, JoinMiter);

    // Draw the main pointer shape and any state indicators (clicks/scroll).
    // The caller should have already restored the underlying pixels from
    // the background snapshot if necessary.
    draw_pointer(display_, target_window_, gc, x_, y_);

    switch (current_state_) {
        case CursorState::LeftClick:
            draw_left_click_indicator(display_, target_window_, gc, x_, y_);
            break;
        case CursorState::RightClick:
            draw_right_click_indicator(display_, target_window_, gc, x_, y_);
            break;
        case CursorState::ScrollUp:
            draw_scroll_up_indicator(display_, target_window_, gc, x_, y_);
            break;
        case CursorState::ScrollDown:
            draw_scroll_down_indicator(display_, target_window_, gc, x_, y_);
            break;
        case CursorState::Normal:
        default:
            break;
    }

    XFreeGC(display_, gc);
}

}  // namespace cursor
}  // namespace turtle
