#include "../include/calculator_button.hpp"

#include <filesystem>
#include <string>

#include <X11/Xlib.h>
#include <Imlib2.h>

namespace turtle {
namespace panel {
namespace {

void draw_icon(Display* display, Window window, const std::string& icon_path, int x, int y, int size) {
    if (!std::filesystem::exists(icon_path)) {
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

}  // namespace

CalculatorButton::CalculatorButton() : icon_path_("images/icons/calculator.png") {}

void CalculatorButton::draw(Display* display, Window window, int x, int y, int width, int height, int icon_size, bool hover) const {
    if (display == nullptr || window == 0) {
        return;
    }

    GC gc = XCreateGC(display, window, 0, nullptr);
    XSetForeground(display, gc, hover ? 0x551AA3FF : 0x33000000);
    XFillRectangle(display, window, gc, x, y, width, height);
    draw_icon(display, window, icon_path_, x + 3, y + 3, icon_size);
    XFreeGC(display, gc);
}

}  // namespace panel
}  // namespace turtle
