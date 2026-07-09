#pragma once

#include <string>

#include <X11/Xlib.h>
#include <Imlib2.h>

namespace turtle {
namespace bckg {

/*
 * BackgroundRenderer
 * ------------------
 * A small, focused wrapper around Imlib2 image loading and rendering. The
 * renderer intentionally exposes only two behaviors:
 * - load_from_file(path): loads and caches the image in memory
 * - render_to_window(display, window, w, h): draws the image scaled to fit
 *
 * The renderer does not own any windowing policy; it simply provides an
 * explicit rendering surface to be used by the WindowManager.
 */

class BackgroundRenderer {
public:
    BackgroundRenderer();
    ~BackgroundRenderer();

    bool load_from_file(const std::string& path);
    bool render_to_window(Display* display, Window window, int width, int height) const;
    bool is_ready() const noexcept;
    const std::string& source_path() const noexcept;

private:
    std::string source_path_;
    Imlib_Image image_;
};

std::string find_default_wallpaper_path();

}  // namespace bckg
}  // namespace turtle
