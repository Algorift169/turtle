#include "../include/panel_placeholder.hpp"

#include <X11/Xlib.h>

namespace turtle {
namespace panel {

void PanelPlaceholder::draw(Display* display, Window window, int x, int y, int width, int height) const {
    if (display == nullptr || window == 0 || width <= 0 || height <= 0) {
        return;
    }

    GC gc = XCreateGC(display, window, 0, nullptr);
    XSetForeground(display, gc, 0x33000000);
    XFillRectangle(display, window, gc, x, y, width, height);
    XFreeGC(display, gc);
}

}  // namespace panel
}  // namespace turtle
