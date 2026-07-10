#pragma once

#include <string>

#include <X11/Xlib.h>

namespace turtle {
namespace panel {

class BatteryButton {
public:
    BatteryButton();
    void draw(Display* display, Window window, int x, int y, int width, int height, int icon_size, int percent, bool hover) const;

private:
    std::string icon_path_;
};

}  // namespace panel
}  // namespace turtle
