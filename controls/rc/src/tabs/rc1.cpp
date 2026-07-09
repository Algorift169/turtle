#include "../../include/rc1.hpp"

namespace turtle {
namespace rc {

RightClickController::RightClickController() = default;

void RightClickController::on_right_click(int x, int y) {
    menu_tab_.open(x, y);
}

bool RightClickController::on_click(int x, int y) {
    return menu_tab_.handle_click(x, y);
}

void RightClickController::draw(Display* display, Window window) const {
    menu_tab_.draw(display, window);
}

bool RightClickController::is_open() const noexcept {
    return menu_tab_.is_open();
}

}  // namespace rc
}  // namespace turtle
