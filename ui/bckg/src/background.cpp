#include "../include/background.hpp"

#include <iostream>
#include <utility>

namespace turtle {
namespace bckg {

namespace {

constexpr const char* kDefaultWallpaperPath = "images/logo/main/main.png";

}  // namespace

BackgroundRenderer::BackgroundRenderer() : image_(nullptr) {}

BackgroundRenderer::~BackgroundRenderer() {
    // Free Imlib2 image resources when the renderer is destroyed. We set the
    // imlib context image to `image_` then request a free+decache which frees
    // both the decoded image and any associated cache entries.
    if (image_ != nullptr) {
        imlib_context_set_image(image_);
        imlib_free_image_and_decache();
        image_ = nullptr;
    }
}

bool BackgroundRenderer::load_from_file(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    if (image_ != nullptr) {
        imlib_context_set_image(image_);
        imlib_free_image_and_decache();
        image_ = nullptr;
    }

    // Attempt to load the requested image using Imlib2. On success we store
    // the source path and keep the decoded image in `image_` for later
    // rendering. On failure the caller can decide whether to fall back or
    // abort initialization.
    image_ = imlib_load_image(path.c_str());
    if (image_ == nullptr) {
        std::cerr << "BackgroundRenderer: failed to load wallpaper from " << path << std::endl;
        return false;
    }

    source_path_ = path;
    return true;
}

bool BackgroundRenderer::render_to_window(Display* display, Window window, int width, int height) const {
    if (display == nullptr || window == 0 || !is_ready() || width <= 0 || height <= 0) {
        return false;
    }

    const int screen = DefaultScreen(display);
    Visual* visual = DefaultVisual(display, screen);
    Colormap colormap = DefaultColormap(display, screen);

    // Configure the imlib context for the target drawable and issue a
    // scaled render of the cached image. We enable blending so images with
    // alpha channels will composite correctly against the drawable.
    imlib_context_set_display(display);
    imlib_context_set_visual(visual);
    imlib_context_set_colormap(colormap);
    imlib_context_set_drawable(window);
    imlib_context_set_image(const_cast<Imlib_Image>(image_));
    imlib_context_set_blend(1);

    imlib_render_image_on_drawable_at_size(0, 0, width, height);
    return true;
}

bool BackgroundRenderer::is_ready() const noexcept {
    return image_ != nullptr;
}

const std::string& BackgroundRenderer::source_path() const noexcept {
    return source_path_;
}

std::string find_default_wallpaper_path() {
    return kDefaultWallpaperPath;
}

}  // namespace bckg
}  // namespace turtle
