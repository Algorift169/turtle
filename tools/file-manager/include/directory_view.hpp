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

private:
    GtkWidget* tree_ = nullptr;
    GtkListStore* store_ = nullptr;
    Activated on_activated_;
    Selected on_selected_;
    // Static callbacks used by GTK signal connections
    static void row_activated_cb(GtkTreeView* tree, GtkTreePath* path, GtkTreeViewColumn* column, gpointer data);
    static void selection_changed_cb(GtkTreeSelection* selection, gpointer data);
    static gboolean button_press_cb(GtkWidget* widget, GdkEventButton* event, gpointer data);
};

}  // namespace turtle::file_manager
