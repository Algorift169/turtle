#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <vector>

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

    void on_file_copy();
    void on_file_paste();
    void on_file_new_window();
    void on_file_create_folder();
    void on_file_create_document();
    void on_file_properties();
    void on_file_close_window();
    void on_file_close_all_windows();

    void on_edit_cut();
    void on_edit_move_to_trash();
    void on_edit_select_all();
    void on_edit_unselect_all();
    void on_edit_rename();
    void on_edit_change_location();

    void on_view_reload();
    void on_view_list_view();
    void on_view_tree_view();
    void on_view_arrange_by(const std::string& mode);
    void on_view_zoom_in();
    void on_view_zoom_out();
    void on_view_hide_icons();

    void on_bookmarks_add_current();
    void on_recent_open(const std::filesystem::path& path);
    void on_help();

    std::vector<std::filesystem::path> bookmarks() const;
    std::vector<std::filesystem::path> recent_items() const;

private:
    void navigate_to(const std::filesystem::path& path);
    void refresh_directory(const std::filesystem::path& path);
    void activate_entry(const FileEntry& entry);
    void select_entry(const FileEntry& entry);
    void mark_recent(const std::filesystem::path& path);
    void load_bookmarks();
    void save_bookmarks();
    void load_recent();
    void save_recent();
    void refresh_menu_bar();
    std::filesystem::path current_location() const;
    std::vector<FileEntry> selected_entries() const;
    FileEntry selected_entry() const;
    GtkWidget* create_menu_bar();
    GtkWidget* create_title_bar();
    void set_status(const std::string& message);
    GtkWidget* window_ = nullptr;
    GtkWidget* root_ = nullptr;
    GtkWidget* menu_bar_ = nullptr;
    GtkWidget* status_ = nullptr;
    FileSystem file_system_;
    NavigationController navigation_;
    std::unique_ptr<Toolbar> toolbar_;
    std::unique_ptr<Sidebar> sidebar_;
    std::unique_ptr<DirectoryView> directory_view_;
    std::unique_ptr<PreviewPanel> preview_panel_;
    std::vector<std::filesystem::path> bookmarks_;
    std::map<std::filesystem::path, int, std::less<>> recent_counts_;
    bool list_view_ = true;
    enum class ClipboardMode { NoneMode, Copy, Cut } clipboard_mode_ = ClipboardMode::NoneMode;
    std::vector<std::filesystem::path> clipboard_items_;
    std::string view_sort_mode_ = "name";
    bool hide_icons_ = false;
};

}  // namespace turtle::file_manager
