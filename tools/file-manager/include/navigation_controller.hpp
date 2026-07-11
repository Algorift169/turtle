#pragma once

#include <filesystem>
#include <functional>
#include <vector>

namespace turtle::file_manager {

// Keeps history policy outside widgets. Views only ask for navigation and
// subscribe to location changes, which makes alternate location UIs possible.
class NavigationController {
public:
    using LocationChanged = std::function<void(const std::filesystem::path&)>;

    explicit NavigationController(std::filesystem::path initial_location);
    void set_location_changed_handler(LocationChanged handler);
    bool navigate_to(const std::filesystem::path& location);
    bool go_back();
    bool go_forward();
    bool go_up();
    bool can_go_back() const noexcept;
    bool can_go_forward() const noexcept;
    const std::filesystem::path& current_location() const noexcept;

private:
    void notify_location_changed();
    std::vector<std::filesystem::path> history_;
    std::size_t history_index_ = 0;
    LocationChanged location_changed_;
};

}  // namespace turtle::file_manager
