#pragma once

#include <filesystem>
#include <functional>

#include <gtk/gtk.h>

namespace turtle::file_manager {

class Sidebar {
public:
    using LocationRequested = std::function<void(const std::filesystem::path&)>;

    explicit Sidebar(LocationRequested on_location_requested);
    GtkWidget* widget() const noexcept;
    void select_row(GtkWidget* row);

private:
    GtkWidget* root_ = nullptr;
    GtkWidget* selected_row_ = nullptr;
};

}  // namespace turtle::file_manager
