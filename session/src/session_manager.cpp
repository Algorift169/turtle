#include "../include/session_manager.hpp"

#include <iostream>

#include "../../ui/wm/include/window_manager.hpp"
#include "../../ui/bckg/include/background.hpp"

namespace turtle {
namespace session {

SessionManager::SessionManager()
    : window_manager_(nullptr), background_(nullptr), running_(false) {}

SessionManager::~SessionManager() {
    shutdown();
}

bool SessionManager::init_window_manager() {
    // Create and initialize the WindowManager. The WM owns its own
    // BackgroundRenderer as well; SessionManager ensures the WM starts up
    // correctly and monitors its readiness.
    window_manager_ = std::make_unique<turtle::wm::WindowManager>();
    if (!window_manager_->initialize()) {
        std::cerr << "SessionManager: WindowManager failed to initialize." << std::endl;
        window_manager_.reset();
        return false;
    }
    return true;
}

bool SessionManager::init_background_manager() {
    // BackgroundRenderer is currently a separate module. We create it here
    // to demonstrate how the SessionManager will manage independent services
    // in the future. Note: the WindowManager also contains a background
    // renderer for immediate rendering; this additional instance is a no-op
    // placeholder used to show session responsibilities without changing the
    // WM internals.
    background_ = std::make_unique<turtle::bckg::BackgroundRenderer>();
    const std::string path = turtle::bckg::find_default_wallpaper_path();
    if (!background_->load_from_file(path)) {
        std::cerr << "SessionManager: BackgroundRenderer failed to load wallpaper: " << path << std::endl;
        background_.reset();
        return false;
    }
    return true;
}

bool SessionManager::initialize() {
    // Initialize components in order. If any step fails, cleanup previously
    // initialized components to avoid leaving partial state.
    if (!init_window_manager()) {
        return false;
    }

    if (!init_background_manager()) {
        // background failed; shutdown window manager then return failure
        if (window_manager_) {
            window_manager_->shutdown();
            window_manager_.reset();
        }
        return false;
    }

    running_ = true;
    return true;
}

int SessionManager::run() {
    if (!running_ || !window_manager_) return 1;

    // The WM currently manages its own event loop. SessionManager keeps
    // responsibility for higher-level lifecycle control. For now we call the
    // WM's run method which will block until it shuts down.
    int rc = window_manager_->run();
    // When WM exits, ensure we perform a graceful shutdown of all services.
    shutdown();
    return rc;
}

void SessionManager::shutdown() {
    if (!running_) return;
    running_ = false;

    // Shutdown order: components are terminated in reverse initialization
    // order to allow dependencies to unwind safely.
    if (background_) {
        background_.reset();
    }

    if (window_manager_) {
        window_manager_->shutdown();
        window_manager_.reset();
    }
}

} // namespace session
} // namespace turtle
