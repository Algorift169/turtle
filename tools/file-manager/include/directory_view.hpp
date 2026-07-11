#pragma once

#include <filesystem>
#include <functional>
#include <vector>

#include <gtk/gtk.h>

#include "file_system.hpp"

namespace turtle::file_manager {

class DirectoryView {
public:
    using Activated = std::function<void(const FileEntry&)>;
    using Selected = std::function<void(const FileEntry&)>;

    DirectoryView(Activated on_activated, Selected on_selected);
    ~DirectoryView();
    GtkWidget* widget() const noexcept;
    void show_entries(const std::vector<FileEntry>& entries);
    void show_error(const std::string& message);
    FileEntry selected_entry() const;
    void select_all_rows();
    void clear_selection();
    void set_zoom_level(int level);
    void set_show_icons(bool show);
    void set_view_mode(bool list_mode);

private:
    GtkWidget* tree_ = nullptr;
    GtkTreeModel* model_ = nullptr;
    GtkListStore* list_store_ = nullptr;
    GtkTreeStore* tree_store_ = nullptr;
    GtkCellRendererPixbuf* icon_renderer_ = nullptr;
    GtkCellRendererText* name_renderer_ = nullptr;
    GtkTreeViewColumn* name_column_ = nullptr;
    GtkTreeViewColumn* size_column_ = nullptr;
    GtkTreeViewColumn* type_column_ = nullptr;
    GtkTreeViewColumn* modified_column_ = nullptr;
    Activated on_activated_;
    Selected on_selected_;
    std::vector<FileEntry> entries_cache_;
    int zoom_level_ = 100;
    bool show_icons_ = true;
    bool list_mode_ = true;
    // Static callbacks used by GTK signal connections
    static void row_activated_cb(GtkTreeView* tree, GtkTreePath* path, GtkTreeViewColumn* column, gpointer data);
    static void selection_changed_cb(GtkTreeSelection* selection, gpointer data);
    static gboolean button_press_cb(GtkWidget* widget, GdkEventButton* event, gpointer data);
};

}  // namespace turtle::file_manager
