#include "../include/panel.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <Imlib2.h>

namespace turtle {
namespace panel {
namespace {

std::string trim(const std::string& input) {
    const auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(begin, end - begin + 1);
}

unsigned long parse_color(const std::string& value) {
    if (value.rfind("0x", 0) == 0) {
        return std::stoul(value.substr(2), nullptr, 16);
    }
    return 0xFFFFFFFF;
}

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

void draw_text(Display* display, Window window, GC gc, int x, int y, const std::string& text) {
    if (text.empty()) {
        return;
    }
    XDrawString(display, window, gc, x, y, text.c_str(), static_cast<int>(text.size()));
}

void draw_icon(Display* display, Window window, const std::string& icon_path, int x, int y, int size) {
    if (icon_path.empty()) {
        return;
    }
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

std::string current_time_text() {
    time_t raw_time = time(nullptr);
    struct tm* time_info = localtime(&raw_time);
    std::ostringstream stream;
    stream << (time_info->tm_hour < 10 ? "0" : "") << time_info->tm_hour << ":"
           << (time_info->tm_min < 10 ? "0" : "") << time_info->tm_min;
    return stream.str();
}

bool point_in_rect(int px, int py, int x, int y, int width, int height) {
    return px >= x && px < x + width && py >= y && py < y + height;
}

int battery_percent() {
    std::ifstream capacity_file("/sys/class/power_supply/BAT0/capacity");
    if (!capacity_file) {
        return 100;
    }

    std::string value;
    std::getline(capacity_file, value);
    return std::max(0, std::min(100, std::stoi(value)));
}

}  // namespace

bool Panel::handle_mouse_press(int x, int y, int desktop_width, int desktop_height) {
    (void)desktop_width;
    const int panel_y = desktop_height - style_.height;
    const int top_y = panel_y + (style_.height - style_.button_size) / 2;
    const int turtle_x = style_.left_padding;
    const int calendar_x = turtle_x + style_.button_size + style_.button_spacing;
    const int browser_x = calendar_x + style_.button_size + style_.button_spacing;
    const int calculator_x = browser_x + style_.button_size + style_.button_spacing;
    const int search_x = calculator_x + style_.button_size + style_.button_spacing;
    const int search_width = style_.search_width;

    if (point_in_rect(x, y, search_x, top_y, search_width, style_.button_size)) {
        search_focused_ = true;
        return true;
    }

    search_focused_ = false;
    return false;
}

bool Panel::handle_key_press(const XKeyEvent& event) {
    if (!search_focused_) {
        return false;
    }

    KeySym key_symbol = XLookupKeysym(const_cast<XKeyEvent*>(&event), 0);
    char buffer[32] = {0};
    int bytes = XLookupString(const_cast<XKeyEvent*>(&event), buffer, sizeof(buffer) - 1, nullptr, nullptr);

    if (key_symbol == XK_BackSpace) {
        if (!search_text_.empty()) {
            search_text_.pop_back();
        }
        return true;
    }

    if (key_symbol == XK_Return || key_symbol == XK_KP_Enter) {
        return true;
    }

    if (key_symbol == XK_Escape) {
        search_focused_ = false;
        return true;
    }

    if (bytes > 0 && std::isprint(static_cast<unsigned char>(buffer[0]))) {
        if (static_cast<int>(search_text_.size()) < 32) {
            search_text_.push_back(buffer[0]);
        }
        return true;
    }

    return false;
}

Panel::Panel() = default;

bool Panel::load_style(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        return false;
    }

    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        const std::size_t separator = trimmed.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim(trimmed.substr(0, separator));
        const std::string value = trim(trimmed.substr(separator + 1));

        if (key == "panel-height") {
            style_.height = std::stoi(value);
        } else if (key == "panel-corner-radius") {
            style_.corner_radius = std::stoi(value);
        } else if (key == "panel-button-size") {
            style_.button_size = std::stoi(value);
        } else if (key == "panel-button-spacing") {
            style_.button_spacing = std::stoi(value);
        } else if (key == "panel-icon-size") {
            style_.icon_size = std::stoi(value);
        } else if (key == "panel-left-padding") {
            style_.left_padding = std::stoi(value);
        } else if (key == "panel-right-padding") {
            style_.right_padding = std::stoi(value);
        } else if (key == "panel-placeholder-margin") {
            style_.placeholder_margin = std::stoi(value);
        } else if (key == "panel-background") {
            style_.background_color = parse_color(value);
        } else if (key == "panel-border") {
            style_.border_color = parse_color(value);
        } else if (key == "panel-text") {
            style_.text_color = parse_color(value);
        }
    }

    return true;
}

void Panel::draw(Display* display, Window window, int desktop_width, int desktop_height, int cursor_x, int cursor_y) const {
    if (display == nullptr || window == 0 || desktop_width <= 0 || desktop_height <= 0) {
        return;
    }

    const int panel_y = desktop_height - style_.height;
    const int panel_width = desktop_width;
    const int panel_height = style_.height;

    GC gc = XCreateGC(display, window, 0, nullptr);
    XSetFillStyle(display, gc, FillSolid);
    XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);

    XSetForeground(display, gc, style_.background_color);
    fill_rounded_rectangle(display, window, gc, 0, panel_y, panel_width, panel_height, style_.corner_radius);

    auto is_hover = [&](int x, int y, int width, int height) {
        return cursor_x >= x && cursor_x < x + width && cursor_y >= y && cursor_y < y + height;
    };

    const int left_x = style_.left_padding;
    const int top_y = panel_y + (panel_height - style_.button_size) / 2;
    const int turtle_x = left_x;
    const int calendar_x = turtle_x + style_.button_size + style_.button_spacing;
    const int browser_x = calendar_x + style_.button_size + style_.button_spacing;
    const int calculator_x = browser_x + style_.button_size + style_.button_spacing;

    const int shutdown_x = desktop_width - style_.right_padding - style_.button_size;
    const int battery_x = shutdown_x - style_.button_size - style_.button_spacing * 2;
    const std::string time_text = current_time_text();
    const int time_text_width = static_cast<int>(time_text.size() * 8);
    const int time_x = battery_x - style_.button_spacing * 4 - time_text_width;
    const int search_x = calculator_x + style_.button_size + style_.button_spacing;
    const int placeholder_x = search_x + style_.search_width + style_.button_spacing;
    const int placeholder_width = time_x - style_.button_spacing - placeholder_x;

    const bool turtle_hover = is_hover(turtle_x, top_y, style_.button_size, style_.button_size);
    const bool calendar_hover = is_hover(calendar_x, top_y, style_.button_size, style_.button_size);
    const bool browser_hover = is_hover(browser_x, top_y, style_.button_size, style_.button_size);
    const bool calculator_hover = is_hover(calculator_x, top_y, style_.button_size, style_.button_size);
    const bool battery_hover = is_hover(battery_x, top_y, style_.button_size, style_.button_size);
    const bool shutdown_hover = is_hover(shutdown_x, top_y, style_.button_size, style_.button_size);

    turtle_button_.draw(display, window, turtle_x, top_y, style_.button_size, style_.button_size, style_.icon_size + 2, turtle_hover);
    calendar_button_.draw(display, window, calendar_x, top_y, style_.button_size, style_.button_size, style_.icon_size, calendar_hover);
    browser_button_.draw(display, window, browser_x, top_y, style_.button_size, style_.button_size, style_.icon_size, browser_hover);
    calculator_button_.draw(display, window, calculator_x, top_y, style_.button_size, style_.button_size, style_.icon_size, calculator_hover);

    const int search_bar_y = top_y;
    const unsigned long search_bg = 0x55333333;
    XSetForeground(display, gc, search_bg);
    XFillRectangle(display, window, gc, search_x, search_bar_y, style_.search_width, style_.button_size);
    XSetForeground(display, gc, 0x88FFFFFF);
    XDrawRectangle(display, window, gc, search_x, search_bar_y, style_.search_width, style_.button_size);
    draw_icon(display, window, "images/icons/find.png", search_x + 6, search_bar_y + 5, style_.button_size - 10);

    const std::string text_to_draw = search_text_.empty() ? "Search..." : search_text_;
    const unsigned long text_color = search_text_.empty() ? 0x99FFFFFF : style_.text_color;
    XSetForeground(display, gc, text_color);
    draw_text(display, window, gc, search_x + style_.button_size + 6, search_bar_y + style_.button_size / 2 + 4, text_to_draw);

    placeholder_.draw(display, window, placeholder_x, panel_y + style_.placeholder_margin, placeholder_width, panel_height - style_.placeholder_margin * 2);

    XSetForeground(display, gc, style_.text_color);
    draw_text(display, window, gc, time_x, panel_y + panel_height / 2 + 4, time_text);

    battery_button_.draw(display, window, battery_x, top_y, style_.button_size, style_.button_size, style_.icon_size, battery_percent(), battery_hover);
    shutdown_button_.draw(display, window, shutdown_x, top_y, style_.button_size, style_.button_size, style_.icon_size, shutdown_hover);

    XFreeGC(display, gc);
}

}  // namespace panel
} // namespace turtle
