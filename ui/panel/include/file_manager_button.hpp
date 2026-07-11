#pragma once

#include <string>

#include <X11/Xlib.h>

namespace turtle {
namespace panel {

class FileManagerButton {
public:
    FileManagerButton();
    void draw(Display* display, Window window, int x, int y, int width, int height, int icon_size, bool hover) const;

private:
    std::string icon_path_;
};

}  // namespace panel
}  // namespace turtle
