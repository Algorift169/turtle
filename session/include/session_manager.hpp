#pragma once

#include <memory>

#include <X11/Xlib.h>

namespace turtle {
namespace wm { class WindowManager; }
namespace bckg { class BackgroundRenderer; }

namespace session {

// SessionManager: central coordinator for Turtle desktop components.
// Responsibilities:
// - initialize subsystems (WM, background, ...)
// - run main loop and monitor for shutdown
// - gracefully shutdown and cleanup resources
// Design notes:
// - Keeps dependencies minimal and uses composition so new services can be
//   registered easily in the future.
class SessionManager {
public:
    SessionManager();
    ~SessionManager();

    // Initialize the session and all required components. Returns true on
    // success; on failure the function will attempt to cleanup partial state
    // before returning false.
    bool initialize();

    // Enter main loop. Returns exit code.
    int run();

    // Request shutdown of the session; safe to call from any thread.
    void shutdown();

private:
    // Initialize individual subsystems. These are broken out to keep
    // `initialize()` readable and to make unit testing easier.
    bool init_window_manager();
    bool init_background_manager();

    // Owned subsystems; forward-declared types are used to avoid header
    // coupling at this level.
    std::unique_ptr<turtle::wm::WindowManager> window_manager_;
    std::unique_ptr<turtle::bckg::BackgroundRenderer> background_;

    bool running_;
};

} // namespace session
} // namespace turtle
