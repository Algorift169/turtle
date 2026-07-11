#include "../include/directory_view.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace turtle::file_manager {
namespace {

enum Column { Icon, Name, Size, Type, Modified, Entry, ColumnCount };

std::string format_size(std::uintmax_t size, EntryKind kind) {
    if (kind == EntryKind::Directory) return "—";
    if (size < 1024) return std::to_string(size) + " B";
    if (size < 1024 * 1024) return std::to_string(size / 1024) + " KB";
    return std::to_string(size / (1024 * 1024)) + " MB";
}

// (callback definitions are implemented below in the turtle::file_manager namespace)

const char* type_name(EntryKind kind) {
    switch (kind) {
        case EntryKind::Directory: return "Folder";
        case EntryKind::Image: return "Image";
        case EntryKind::Text: return "Text file";
        case EntryKind::Video: return "Video";
        default: return "File";
    }
}

std::string modified_text(const std::filesystem::path& path) {
    std::error_code error;
    const auto timestamp = std::filesystem::last_write_time(path, error);
    if (error) return "—";
    const auto system_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        timestamp - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    const std::time_t value = std::chrono::system_clock::to_time_t(system_time);
    std::tm local{};
    localtime_r(&value, &local);
    std::ostringstream result;
    result << std::put_time(&local, "%Y-%m-%d %H:%M");
    return result.str();
}

GdkPixbuf* entry_icon(const FileEntry& entry) {
    const char* path = entry.kind == EntryKind::Directory ? "images/icons/folder.svg" : "";
    GError* error = nullptr;
    if (*path != '\0') {
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale(path, 22, 22, TRUE, &error);
        if (pixbuf != nullptr) return pixbuf;
        if (error != nullptr) g_error_free(error);
    }
    const char* fallback = entry.kind == EntryKind::Image ? "image-x-generic" : "text-x-generic";
    GtkIconInfo* info = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(), fallback, 22, GTK_ICON_LOOKUP_FORCE_SIZE);
    if (info == nullptr) return nullptr;
    GdkPixbuf* pixbuf = gtk_icon_info_load_icon(info, &error);
    g_object_unref(info);
    if (error != nullptr) g_error_free(error);
    return pixbuf;
}

void clear_entries(GtkListStore* store) {
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    while (valid) {
        FileEntry* entry = nullptr;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, Entry, &entry, -1);
        delete entry;
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }
    gtk_list_store_clear(store);
}

}  // namespace

// Define the static DirectoryView callbacks in the enclosing namespace so
// they can access private members of the class.
void DirectoryView::row_activated_cb(GtkTreeView* /*tree*/, GtkTreePath* path, GtkTreeViewColumn* /*column*/, gpointer data) {
    auto* view = static_cast<DirectoryView*>(data);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(view->store_), &iter, path)) return;
    FileEntry* entry = nullptr;
    gtk_tree_model_get(GTK_TREE_MODEL(view->store_), &iter, Entry, &entry, -1);
    if (entry) view->on_activated_(*entry);
}

void DirectoryView::selection_changed_cb(GtkTreeSelection* selection, gpointer data) {
    auto* view = static_cast<DirectoryView*>(data);
    GtkTreeModel* model = nullptr;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) return;
    FileEntry* entry = nullptr;
    gtk_tree_model_get(model, &iter, Entry, &entry, -1);
    if (entry) view->on_selected_(*entry);
}

gboolean DirectoryView::button_press_cb(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    if (event->type != GDK_BUTTON_PRESS || event->button != 1) return FALSE;
    auto* view = static_cast<DirectoryView*>(data);
    int x = static_cast<int>(event->x);
    int y = static_cast<int>(event->y);
    GtkTreePath* path = nullptr;
    GtkTreeViewColumn* column = nullptr;
    int cell_x = 0, cell_y = 0;
    if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), x, y, &path, &column, &cell_x, &cell_y)) return FALSE;
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(view->store_), &iter, path)) {
        gtk_tree_path_free(path);
        return FALSE;
    }
    FileEntry* entry = nullptr;
    gtk_tree_model_get(GTK_TREE_MODEL(view->store_), &iter, Entry, &entry, -1);
    gtk_tree_path_free(path);
    if (entry && entry->kind == EntryKind::Directory) {
        view->on_activated_(*entry);
    }
    return FALSE;
}

DirectoryView::DirectoryView(Activated on_activated, Selected on_selected)
    : on_activated_(std::move(on_activated)), on_selected_(std::move(on_selected)) {
    store_ = gtk_list_store_new(ColumnCount, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
    tree_ = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_), TRUE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree_), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree_), Name);

    const auto add_text_column = [this](const char* title, Column column, int expand) {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn* view_column = gtk_tree_view_column_new_with_attributes(title, renderer, "text", column, nullptr);
        gtk_tree_view_column_set_expand(view_column, expand);
        gtk_tree_view_column_set_sort_column_id(view_column, column);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_), view_column);
    };
    GtkCellRenderer* icon_renderer = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn* name_column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(name_column, "Name");
    gtk_tree_view_column_pack_start(name_column, icon_renderer, FALSE);
    gtk_tree_view_column_add_attribute(name_column, icon_renderer, "pixbuf", Icon);
    GtkCellRenderer* name_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(name_column, name_renderer, TRUE);
    gtk_tree_view_column_add_attribute(name_column, name_renderer, "text", Name);
    gtk_tree_view_column_set_expand(name_column, TRUE);
    gtk_tree_view_column_set_sort_column_id(name_column, Name);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_), name_column);
    add_text_column("Size", Size, FALSE);
    add_text_column("Type", Type, FALSE);
    add_text_column("Modified", Modified, FALSE);

    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(tree_, "row-activated", G_CALLBACK(DirectoryView::row_activated_cb), this);
    g_signal_connect(selection, "changed", G_CALLBACK(DirectoryView::selection_changed_cb), this);

    g_signal_connect(tree_, "button-press-event", G_CALLBACK(DirectoryView::button_press_cb), this);
}

DirectoryView::~DirectoryView() { clear_entries(store_); g_object_unref(store_); }

GtkWidget* DirectoryView::widget() const noexcept { return tree_; }

void DirectoryView::show_entries(const std::vector<FileEntry>& entries) {
    clear_entries(store_);
    for (const auto& entry : entries) {
        GtkTreeIter row; gtk_list_store_append(store_, &row);
        GdkPixbuf* icon = entry_icon(entry);
        gtk_list_store_set(store_, &row, Icon, icon, Name, entry.name.c_str(), Size, format_size(entry.size, entry.kind).c_str(),
                           Type, type_name(entry.kind), Modified, modified_text(entry.path).c_str(), Entry, new FileEntry(entry), -1);
        if (icon != nullptr) g_object_unref(icon);
    }
}

void DirectoryView::show_error(const std::string& message) {
    clear_entries(store_); GtkTreeIter row; gtk_list_store_append(store_, &row);
    gtk_list_store_set(store_, &row, Name, message.c_str(), Type, "Error", -1);
}

}  // namespace turtle::file_manager
