#pragma once

#include <X11/Xlib.h>

namespace turtle {
namespace panel {

class PanelPlaceholder {
public:
    PanelPlaceholder() = default;
    void draw(Display* display, Window window, int x, int y, int width, int height) const;
};

}  // namespace panel
}  // namespace turtle
