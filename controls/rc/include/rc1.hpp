#pragma once

#include "../../../ui/tabs/include/w-tab.hpp"
#include <X11/Xlib.h>

namespace turtle {
namespace rc {

class RightClickController {
public:
    RightClickController();
    ~RightClickController() = default;

    void on_right_click(int x, int y);
    bool on_click(int x, int y);
    void draw(Display* display, Window window) const;
    bool is_open() const noexcept;

private:
    tabs::RightClickTab menu_tab_;
};

}  // namespace rc
}  // namespace turtle
