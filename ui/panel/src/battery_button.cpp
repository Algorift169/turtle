#include "../include/battery_button.hpp"

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

BatteryButton::BatteryButton() : icon_path_("") {}

void BatteryButton::draw(Display* display, Window window, int x, int y, int width, int height, int icon_size, int percent, bool hover) const {
    if (display == nullptr || window == 0) {
        return;
    }

    GC gc = XCreateGC(display, window, 0, nullptr);
    XSetForeground(display, gc, hover ? 0x5520B2FF : 0x33000000);
    XFillRectangle(display, window, gc, x, y, width, height);
    XSetForeground(display, gc, 0xFFFFFFFF);
    std::string text = std::to_string(percent) + "%";
    int text_width = static_cast<int>(text.size() * 7);
    int text_x = x + (width - text_width) / 2;
    int text_y = y + (height + 8) / 2;
    XDrawString(display, window, gc, text_x, text_y, text.c_str(), static_cast<int>(text.size()));
    XFreeGC(display, gc);
}

}  // namespace panel
}  // namespace turtle
