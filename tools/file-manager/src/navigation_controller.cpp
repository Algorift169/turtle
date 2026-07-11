#include "../include/navigation_controller.hpp"

namespace turtle::file_manager {

NavigationController::NavigationController(std::filesystem::path initial_location) : history_{std::move(initial_location)} {}
void NavigationController::set_location_changed_handler(LocationChanged handler) { location_changed_ = std::move(handler); }
bool NavigationController::navigate_to(const std::filesystem::path& location) {
    if (location.empty() || location == current_location()) return false;
    history_.erase(history_.begin() + static_cast<long>(history_index_ + 1), history_.end());
    history_.push_back(location); history_index_ = history_.size() - 1; notify_location_changed(); return true;
}
bool NavigationController::go_back() { if (!can_go_back()) return false; --history_index_; notify_location_changed(); return true; }
bool NavigationController::go_forward() { if (!can_go_forward()) return false; ++history_index_; notify_location_changed(); return true; }
bool NavigationController::go_up() { const auto parent = current_location().parent_path(); return !parent.empty() && parent != current_location() && navigate_to(parent); }
bool NavigationController::can_go_back() const noexcept { return history_index_ > 0; }
bool NavigationController::can_go_forward() const noexcept { return history_index_ + 1 < history_.size(); }
const std::filesystem::path& NavigationController::current_location() const noexcept { return history_[history_index_]; }
void NavigationController::notify_location_changed() { if (location_changed_) location_changed_(current_location()); }

}  // namespace turtle::file_manager
