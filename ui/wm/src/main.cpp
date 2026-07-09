#include "../../session/include/session_manager.hpp"

#include <iostream>

int main() {
    turtle::session::SessionManager session;

    if (!session.initialize()) {
        std::cerr << "Main: failed to initialize the Turtle session." << std::endl;
        return 1;
    }

    return session.run();
}
