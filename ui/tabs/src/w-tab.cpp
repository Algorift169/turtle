#include "../include/w-tab.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Imlib2.h>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

void fill_rounded_rectangle(Display* display, Window window, GC gc,
                            int x, int y, int width, int height, int radius) {
    const int diameter = radius * 2;
    XFillRectangle(display, window, gc, x + radius, y, width - diameter, height);
    XFillRectangle(display, window, gc, x, y + radius, radius, height - diameter);
    XFillRectangle(display, window, gc, x + width - radius, y + radius, radius, height - diameter);
    XFillRectangle(display, window, gc, x + radius, y + height - radius, width - diameter, radius);

    XFillArc(display, window, gc, x, y, diameter, diameter, 0, 23040);
    XFillArc(display, window, gc, x + width - diameter, y, diameter, diameter, 0, 23040);
    XFillArc(display, window, gc, x, y + height - diameter, diameter, diameter, 0, 23040);
    XFillArc(display, window, gc, x + width - diameter, y + height - diameter, diameter, diameter, 0, 23040);
}

void draw_rounded_rectangle(Display* display, Window window, GC gc,
                            int x, int y, int width, int height, int radius) {
    const int diameter = radius * 2;
    XDrawArc(display, window, gc, x, y, diameter, diameter, 0, 23040);
    XDrawArc(display, window, gc, x + width - diameter, y, diameter, diameter, 0, 23040);
    XDrawArc(display, window, gc, x, y + height - diameter, diameter, diameter, 0, 23040);
    XDrawArc(display, window, gc, x + width - diameter, y + height - diameter, diameter, diameter, 0, 23040);
    XDrawRectangle(display, window, gc, x + radius, y, width - diameter, height);
    XDrawRectangle(display, window, gc, x, y + radius, width, height - diameter);
}

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

namespace turtle {
namespace tabs {

RightClickTab::RightClickTab()
    : x_(0), y_(0), width_(180), height_(0), visible_(false) {
    build_items();
}

void RightClickTab::build_items() {
    items_.clear();
    const std::vector<std::pair<std::string, std::string>> entries = {
        //{"Create Launcher", "images/icons/other.svg"}, TODO
        {"Create Folder", "images/icons/folder.svg"},
        {"Create Document", "images/icons/text-editor.png"},
        {"Terminal", "images/icons/terminal.svg"},
        {"Refresh", "images/icons/refresh.svg"},
        {"Display Settings", "images/icons/desktop.svg"},
        {"Applications", "images/icons/applications.svg"},
        {"Settings","images/icons/settings.svg"},
    };

    const int item_height = 22;
    const int padding = 12;
    const int vertical_spacing = 6;
    int item_y = padding;
    for (const auto& entry : entries) {
        items_.push_back(MenuItem{entry.first, entry.second, padding, item_y, width_ - padding * 2, item_height});
        item_y += item_height + vertical_spacing;
    }
    height_ = item_y + padding - vertical_spacing;
}

bool RightClickTab::is_open() const noexcept {
    return visible_;
}

void RightClickTab::open(int mouse_x, int mouse_y) {
    x_ = mouse_x;
    y_ = mouse_y;
    visible_ = true;
}

void RightClickTab::close() noexcept {
    visible_ = false;
}

bool RightClickTab::contains_point(int point_x, int point_y) const noexcept {
    if (!visible_) {
        return false;
    }
    return point_x >= x_ && point_x <= x_ + width_ && point_y >= y_ && point_y <= y_ + height_;
}

int RightClickTab::find_item(int x, int y) const noexcept {
    if (!visible_) {
        return -1;
    }

    const int local_x = x - x_;
    const int local_y = y - y_;
    for (size_t index = 0; index < items_.size(); ++index) {
        const auto& item = items_[index];
        if (local_x >= item.x && local_x <= item.x + item.width && local_y >= item.y && local_y <= item.y + item.height) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

bool RightClickTab::handle_click(int click_x, int click_y) {
    if (!visible_) {
        return false;
    }

    if (!contains_point(click_x, click_y)) {
        close();
        return false;
    }

    const int item_index = find_item(click_x, click_y);
    if (item_index >= 0) {
        // For now we only close the menu. Action handling will be implemented later.
        close();
        return true;
    }

    return false;
}

void RightClickTab::draw(Display* display, Window window) const {
    if (!visible_) {
        return;
    }

    const GC gc = XCreateGC(display, window, 0, nullptr);
    XSetFillStyle(display, gc, FillSolid);
    XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);

    const int radius = 12;
    const unsigned long bg_color = 0x22000000; // dark translucent background
    const unsigned long text_color = WhitePixel(display, DefaultScreen(display));

    XSetForeground(display, gc, bg_color);
    fill_rounded_rectangle(display, window, gc, x_, y_, width_, height_, radius);

    XSetForeground(display, gc, text_color);
    for (const auto& item : items_) {
        const int icon_x = x_ + item.x + 6;
        const int icon_y = y_ + item.y + 2;
        const int icon_size = 16;
        draw_icon(display, window, item.icon_path, icon_x, icon_y, icon_size);

        const int text_x = x_ + item.x + 28;
        const int text_y = y_ + item.y + item.height - 8;
        XDrawString(display, window, gc, text_x, text_y,
                    item.label.c_str(), static_cast<int>(item.label.size()));
    }

    XFreeGC(display, gc);
}

}  // namespace tabs
}  // namespace turtle
