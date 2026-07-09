#include "../include/window_manager.hpp"

#include <iostream>

int main() {
    turtle::wm::WindowManager window_manager;

    if (!window_manager.initialize()) {
        std::cerr << "Main: failed to initialize the Turtle window manager." << std::endl;
        return 1;
    }

    return window_manager.run();
}
