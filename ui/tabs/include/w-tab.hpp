#pragma once

#include <X11/Xlib.h>
#include <string>
#include <vector>

namespace turtle {
namespace tabs {

struct MenuItem {
    std::string label;
    int x;
    int y;
    int width;
    int height;
};

class RightClickTab {
public:
    RightClickTab();

    bool is_open() const noexcept;
    void open(int mouse_x, int mouse_y);
    void close() noexcept;
    bool handle_click(int click_x, int click_y);
    bool contains_point(int point_x, int point_y) const noexcept;
    void draw(Display* display, Window window) const;

private:
    void build_items();
    int find_item(int x, int y) const noexcept;

    std::vector<MenuItem> items_;
    int x_;
    int y_;
    int width_;
    int height_;
    bool visible_;
};

}  // namespace tabs
}  // namespace turtle
