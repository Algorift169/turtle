#pragma once

#include <string>

#include <X11/Xlib.h>

#include "turtle_button.hpp"
#include "calendar_button.hpp"
#include "browser_button.hpp"
#include "calculator_button.hpp"
#include "battery_button.hpp"
#include "shutdown_button.hpp"
#include "panel_placeholder.hpp"

namespace turtle {
namespace panel {

class Panel {
public:
    Panel();
    bool load_style(const std::string& path = "style/panel/panel.style");
    void draw(Display* display, Window window, int desktop_width, int desktop_height, int cursor_x, int cursor_y) const;
    bool handle_mouse_press(int x, int y, int desktop_width, int desktop_height);
    bool handle_key_press(const XKeyEvent& event);

private:
    struct Style {
        int height = 35;
        int corner_radius = 0;
        int button_size = 28;
        int button_spacing = 6;
        int icon_size = 20;
        int search_width = 160;
        int left_padding = 0;
        int right_padding = 0;
        int placeholder_margin = 0;
        unsigned long background_color = 0xCC000000;
        unsigned long border_color = 0x00000000;
        unsigned long text_color = 0xFFFFFFFF;
    };

    Style style_;
    TurtleButton turtle_button_;
    CalendarButton calendar_button_;
    BrowserButton browser_button_;
    CalculatorButton calculator_button_;
    BatteryButton battery_button_;
    ShutdownButton shutdown_button_;
    PanelPlaceholder placeholder_;
    bool search_focused_ = false;
    std::string search_text_;
};

}  // namespace panel
}  // namespace turtle
