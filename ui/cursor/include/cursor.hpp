#pragma once

#include <X11/Xlib.h>

namespace turtle {
namespace cursor {

enum class CursorState {
    Normal,
    LeftClick,
    RightClick,
    ScrollUp,
    ScrollDown,
};

class CursorRenderer {
public:
    CursorRenderer();
    ~CursorRenderer();

    bool initialize(Display* display, Window window);
    void set_state(CursorState state);
    void set_position(int x, int y);
    void draw_overlay() const;

private:
    void destroy_cursor();
    

    Display* display_;
    Window target_window_;
    Window root_window_;
    Cursor cursor_;
    CursorState current_state_;
    int x_;
    int y_;
};

}  // namespace cursor
}  // namespace turtle
