#pragma once

#include <filesystem>
#include <memory>

#include <gtk/gtk.h>

#include "directory_view.hpp"
#include "file_system.hpp"
#include "navigation_controller.hpp"
#include "preview_panel.hpp"
#include "sidebar.hpp"
#include "toolbar.hpp"

namespace turtle::file_manager {

// Composition root for the application: this class wires GTK components to
// filesystem and navigation services without giving child widgets global state.
class FileManagerWindow {
public:
    FileManagerWindow();
    void show();

private:
    void navigate_to(const std::filesystem::path& path);
    void refresh_directory(const std::filesystem::path& path);
    void activate_entry(const FileEntry& entry);
    void select_entry(const FileEntry& entry);
    GtkWidget* create_menu_bar();
    GtkWidget* create_title_bar();
    void set_status(const std::string& message);
    GtkWidget* window_ = nullptr;
    GtkWidget* status_ = nullptr;
    FileSystem file_system_;
    NavigationController navigation_;
    std::unique_ptr<Toolbar> toolbar_;
    std::unique_ptr<Sidebar> sidebar_;
    std::unique_ptr<DirectoryView> directory_view_;
    std::unique_ptr<PreviewPanel> preview_panel_;
};

}  // namespace turtle::file_manager
