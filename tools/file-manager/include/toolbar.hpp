#pragma once

#include <functional>
#include <string>

#include <gtk/gtk.h>

namespace turtle::file_manager {

class Toolbar {
public:
    struct Actions {
        std::function<void()> back;
        std::function<void()> forward;
        std::function<void()> up;
        std::function<void()> home;
        std::function<void()> refresh;
    };

    explicit Toolbar(Actions actions);
    GtkWidget* widget() const noexcept;
    void set_path(const std::string& path);
    void set_navigation_state(bool can_go_back, bool can_go_forward);

private:
    GtkWidget* root_ = nullptr;
    GtkWidget* back_ = nullptr;
    GtkWidget* forward_ = nullptr;
    GtkWidget* home_ = nullptr;
    GtkWidget* refresh_ = nullptr;
    GtkWidget* path_ = nullptr;
};

}  // namespace turtle::file_manager
