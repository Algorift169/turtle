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
#include <unistd.h>

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

int battery_percent() {
    std::ifstream capacity_file("/sys/class/power_supply/BAT0/capacity");
    if (!capacity_file) {
        return 100;
    }

    std::string value;
    std::getline(capacity_file, value);
    return std::max(0, std::min(100, std::stoi(value)));
}

int cpu_usage_percent() {
    // Track previous /proc/stat values between calls to compute a delta.
    static long prev_idle = 0;
    static long prev_total = 0;

    std::ifstream stat_file("/proc/stat");
    if (!stat_file) {
        return 0;
    }

    std::string label;
    stat_file >> label;  // consume "cpu"
    long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
    stat_file >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    const long total_idle = idle + iowait;
    const long total = user + nice + system + idle + iowait + irq + softirq + steal;

    const long diff_idle = total_idle - prev_idle;
    const long diff_total = total - prev_total;

    prev_idle = total_idle;
    prev_total = total;

    if (diff_total == 0) {
        return 0;
    }
    return static_cast<int>(100 * (diff_total - diff_idle) / diff_total);
}

int memory_usage_percent() {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo) {
        return 0;
    }

    long mem_total = 0;
    long mem_available = 0;
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.rfind("MemTotal:", 0) == 0) {
            std::istringstream iss(line);
            std::string key;
            iss >> key >> mem_total;
        } else if (line.rfind("MemAvailable:", 0) == 0) {
            std::istringstream iss(line);
            std::string key;
            iss >> key >> mem_available;
            break;  // MemAvailable always appears after MemTotal
        }
    }

    if (mem_total == 0) {
        return 0;
    }
    return static_cast<int>(100 * (mem_total - mem_available) / mem_total);
}

int process_count() {
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        const std::string name = entry.path().filename().string();
        if (!name.empty() && std::isdigit(static_cast<unsigned char>(name[0]))) {
            ++count;
        }
    }
    return count;
}

}  // namespace

bool Panel::handle_mouse_press(int x, int y, int desktop_width, int desktop_height) {
    (void)desktop_width;
    const int panel_y = 0;
    const int top_y = panel_y + (style_.height - style_.button_size) / 2;
    const int turtle_x = style_.left_padding;
    const int calendar_x = turtle_x + style_.button_size + style_.button_spacing;
    const int browser_x = calendar_x + style_.button_size + style_.button_spacing;
    const int calculator_x = browser_x + style_.button_size + style_.button_spacing;
    const int file_manager_x = calculator_x + style_.button_size + style_.button_spacing;
    const int search_x = file_manager_x + style_.button_size + style_.button_spacing;
    const int search_width = style_.search_width;

    if (point_in_rect(x, y, file_manager_x, top_y, style_.button_size, style_.button_size)) {
        launch_file_manager();
        return true;
    }

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

    const int panel_y = 0;
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
    const int file_manager_x = calculator_x + style_.button_size + style_.button_spacing;

    const int shutdown_x = desktop_width - style_.right_padding - style_.button_size;
    const int battery_x = shutdown_x - style_.button_size - style_.button_spacing * 2;
    const std::string time_text = current_time_text();
    const int time_text_width = static_cast<int>(time_text.size() * 8);
    const int time_x = battery_x - style_.button_spacing * 4 - time_text_width;

    // Build live system-stat strings.
    const std::string cpu_text = "cpu: " + std::to_string(cpu_usage_percent()) + "%";
    const std::string mem_text = "mem: " + std::to_string(memory_usage_percent()) + "%";
    const std::string proc_text = "processes: " + std::to_string(process_count());
    const int char_w = 7;  // approximate per-character width for the default X font
    const int stat_spacing = style_.button_spacing * 2;
    const int cpu_text_width = static_cast<int>(cpu_text.size()) * char_w;
    const int mem_text_width = static_cast<int>(mem_text.size()) * char_w;
    const int proc_text_width = static_cast<int>(proc_text.size()) * char_w;
    const int stats_total_width = cpu_text_width + stat_spacing + mem_text_width + stat_spacing + proc_text_width + stat_spacing;

    // Position the stats block just before the time.
    const int stats_x = time_x - stats_total_width;

    const int search_x = calculator_x + style_.button_size + style_.button_spacing;
    const int search_height = std::max(20, style_.button_size - 10);
    const int search_bar_y = top_y + (style_.button_size - search_height) / 2;
    const int placeholder_x = search_x + style_.search_width + style_.button_spacing;
    const int placeholder_width = stats_x - style_.button_spacing - placeholder_x;

    const bool turtle_hover = is_hover(turtle_x, top_y, style_.button_size, style_.button_size);
    const bool calendar_hover = is_hover(calendar_x, top_y, style_.button_size, style_.button_size);
    const bool browser_hover = is_hover(browser_x, top_y, style_.button_size, style_.button_size);
    const bool calculator_hover = is_hover(calculator_x, top_y, style_.button_size, style_.button_size);
    const bool file_manager_hover = is_hover(file_manager_x, top_y, style_.button_size, style_.button_size);
    const bool battery_hover = is_hover(battery_x, top_y, style_.button_size, style_.button_size);
    const bool shutdown_hover = is_hover(shutdown_x, top_y, style_.button_size, style_.button_size);

    turtle_button_.draw(display, window, turtle_x, top_y, style_.button_size, style_.button_size, style_.icon_size + 2, turtle_hover);
    calendar_button_.draw(display, window, calendar_x, top_y, style_.button_size, style_.button_size, style_.icon_size, calendar_hover);
    browser_button_.draw(display, window, browser_x, top_y, style_.button_size, style_.button_size, style_.icon_size, browser_hover);
    calculator_button_.draw(display, window, calculator_x, top_y, style_.button_size, style_.button_size, style_.icon_size, calculator_hover);
    file_manager_button_.draw(display, window, file_manager_x, top_y, style_.button_size, style_.button_size, style_.icon_size, file_manager_hover);

    const unsigned long search_bg = 0x55333333;
    XSetForeground(display, gc, search_bg);
    XFillRectangle(display, window, gc, search_x, search_bar_y, style_.search_width, search_height);
    XSetForeground(display, gc, 0x88FFFFFF);
    XDrawRectangle(display, window, gc, search_x, search_bar_y, style_.search_width, search_height);
    draw_icon(display, window, "images/icons/find.png", search_x + 6, search_bar_y + 4, search_height - 10);

    const std::string text_to_draw = search_text_.empty() ? "Search..." : search_text_;
    const unsigned long text_color = search_text_.empty() ? 0x99FFFFFF : style_.text_color;
    XSetForeground(display, gc, text_color);
    draw_text(display, window, gc, search_x + style_.button_size + 6, search_bar_y + search_height / 2 + 4, text_to_draw);

    placeholder_.draw(display, window, placeholder_x, panel_y + style_.placeholder_margin, placeholder_width, panel_height - style_.placeholder_margin * 2);

    // Draw live system stats: cpu | mem | processes
    const int text_y = panel_y + panel_height / 2 + 4;
    const unsigned long stat_label_color = 0xAAAAAAFF;  // subtle blueish-grey for labels
    int sx = stats_x;

    XSetForeground(display, gc, stat_label_color);
    draw_text(display, window, gc, sx, text_y, cpu_text);
    sx += cpu_text_width + stat_spacing;

    draw_text(display, window, gc, sx, text_y, mem_text);
    sx += mem_text_width + stat_spacing;

    draw_text(display, window, gc, sx, text_y, proc_text);

    // Draw time
    XSetForeground(display, gc, style_.text_color);
    draw_text(display, window, gc, time_x, text_y, time_text);

    battery_button_.draw(display, window, battery_x, top_y, style_.button_size, style_.button_size, style_.icon_size, battery_percent(), battery_hover);
    shutdown_button_.draw(display, window, shutdown_x, top_y, style_.button_size, style_.button_size, style_.icon_size, shutdown_hover);

    XFreeGC(display, gc);
}

}  // namespace panel
} // namespace turtle
