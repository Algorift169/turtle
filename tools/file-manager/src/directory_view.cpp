#include "../include/directory_view.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
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

GdkPixbuf* entry_icon(const FileEntry& entry, int size) {
    const char* path = entry.kind == EntryKind::Directory ? "images/icons/folder.svg" : "";
    GError* error = nullptr;
    if (*path != '\0') {
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &error);
        if (pixbuf != nullptr) return pixbuf;
        if (error != nullptr) g_error_free(error);
    }
    const char* fallback = entry.kind == EntryKind::Image ? "image-x-generic" : "text-x-generic";
    GtkIconInfo* info = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(), fallback, size, GTK_ICON_LOOKUP_FORCE_SIZE);
    if (info == nullptr) return nullptr;
    GdkPixbuf* pixbuf = gtk_icon_info_load_icon(info, &error);
    g_object_unref(info);
    if (error != nullptr) g_error_free(error);
    return pixbuf;
}

void clear_list_store(GtkListStore* store) {
    if (store == nullptr) return;
    gtk_list_store_clear(store);
}

void clear_tree_store(GtkTreeStore* store) {
    if (store == nullptr) return;
    gtk_tree_store_clear(store);
}

std::size_t row_index_from_path(GtkTreePath* path, bool list_mode) {
    if (path == nullptr) return 0;
    const auto* indices = gtk_tree_path_get_indices(path);
    if (indices == nullptr) return 0;
    const int depth = gtk_tree_path_get_depth(path);
    if (list_mode) return static_cast<std::size_t>(indices[0]);
    if (depth > 1) return static_cast<std::size_t>(indices[depth - 1]);
    return 0;
}

gpointer make_entry_tag(std::size_t index) {
    return reinterpret_cast<gpointer>(static_cast<std::uintptr_t>(index + 1));
}

std::size_t entry_tag_to_index(gpointer value) {
    return value == nullptr ? 0 : static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(value) - 1);
}

}  // namespace

// Define the static DirectoryView callbacks in the enclosing namespace so
// they can access private members of the class.
void DirectoryView::row_activated_cb(GtkTreeView* /*tree*/, GtkTreePath* path, GtkTreeViewColumn* /*column*/, gpointer data) {
    auto* view = static_cast<DirectoryView*>(data);
    if (view == nullptr || view->entries_cache_.empty()) return;
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(view->model_, &iter, path)) return;
    const auto index = row_index_from_path(path, view->list_mode_);
    if (index >= view->entries_cache_.size()) return;
    const auto& entry = view->entries_cache_[index];
    if (entry.kind == EntryKind::Directory) {
        view->on_activated_(entry);
    } else {
        view->on_selected_(entry);
    }
}

void DirectoryView::selection_changed_cb(GtkTreeSelection* selection, gpointer data) {
    auto* view = static_cast<DirectoryView*>(data);
    if (view == nullptr || view->entries_cache_.empty()) return;
    GtkTreeModel* model = nullptr;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) return;
    GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
    if (path == nullptr) return;
    const auto index = row_index_from_path(path, view->list_mode_);
    gtk_tree_path_free(path);
    if (index < view->entries_cache_.size()) {
        view->on_selected_(view->entries_cache_[index]);
    }
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
    if (!gtk_tree_model_get_iter(view->model_, &iter, path)) {
        gtk_tree_path_free(path);
        return FALSE;
    }
    const auto index = row_index_from_path(path, view->list_mode_);
    gtk_tree_path_free(path);
    if (index < view->entries_cache_.size()) {
        const auto& entry = view->entries_cache_[index];
        if (entry.kind == EntryKind::Directory) {
            view->on_activated_(entry);
        }
    }
    return FALSE;
}

DirectoryView::DirectoryView(Activated on_activated, Selected on_selected)
    : on_activated_(std::move(on_activated)), on_selected_(std::move(on_selected)) {
    list_store_ = gtk_list_store_new(ColumnCount, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
    tree_store_ = gtk_tree_store_new(ColumnCount, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
    model_ = GTK_TREE_MODEL(list_store_);
    tree_ = gtk_tree_view_new_with_model(model_);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_), TRUE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree_), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree_), Name);

    const auto add_text_column = [this](const char* title, Column column, int expand) {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn* view_column = gtk_tree_view_column_new_with_attributes(title, renderer, "text", column, nullptr);
        gtk_tree_view_column_set_expand(view_column, expand);
        gtk_tree_view_column_set_sort_column_id(view_column, column);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_), view_column);
        return view_column;
    };
    icon_renderer_ = GTK_CELL_RENDERER_PIXBUF(gtk_cell_renderer_pixbuf_new());
    name_renderer_ = GTK_CELL_RENDERER_TEXT(gtk_cell_renderer_text_new());
    name_column_ = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(name_column_, "Name");
    gtk_tree_view_column_pack_start(name_column_, GTK_CELL_RENDERER(icon_renderer_), FALSE);
    gtk_tree_view_column_add_attribute(name_column_, GTK_CELL_RENDERER(icon_renderer_), "pixbuf", Icon);
    gtk_tree_view_column_pack_start(name_column_, GTK_CELL_RENDERER(name_renderer_), TRUE);
    gtk_tree_view_column_add_attribute(name_column_, GTK_CELL_RENDERER(name_renderer_), "text", Name);
    gtk_tree_view_column_set_expand(name_column_, TRUE);
    gtk_tree_view_column_set_sort_column_id(name_column_, Name);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_), name_column_);
    size_column_ = add_text_column("Size", Size, FALSE);
    type_column_ = add_text_column("Type", Type, FALSE);
    modified_column_ = add_text_column("Modified", Modified, FALSE);

    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(tree_, "row-activated", G_CALLBACK(DirectoryView::row_activated_cb), this);
    g_signal_connect(selection, "changed", G_CALLBACK(DirectoryView::selection_changed_cb), this);

    g_signal_connect(tree_, "button-press-event", G_CALLBACK(DirectoryView::button_press_cb), this);
}

DirectoryView::~DirectoryView() {
    if (list_store_ != nullptr) {
        clear_list_store(list_store_);
        g_object_unref(list_store_);
    }
    if (tree_store_ != nullptr) {
        clear_tree_store(tree_store_);
        g_object_unref(tree_store_);
    }
}

GtkWidget* DirectoryView::widget() const noexcept { return tree_; }

void DirectoryView::show_entries(const std::vector<FileEntry>& entries) {
    entries_cache_ = entries;
    std::vector<FileEntry> ordered = entries;
    std::sort(ordered.begin(), ordered.end(), [](const FileEntry& left, const FileEntry& right) {
        if (left.kind != right.kind) return left.kind == EntryKind::Directory && right.kind != EntryKind::Directory;
        return left.name < right.name;
    });
    if (list_mode_) {
        clear_list_store(list_store_);
        for (const auto& entry : ordered) {
            GtkTreeIter row;
            gtk_list_store_append(list_store_, &row);
            const int icon_size = std::max(16, zoom_level_ * 22 / 100);
            GdkPixbuf* icon = show_icons_ ? entry_icon(entry, icon_size) : nullptr;
            gtk_list_store_set(list_store_, &row, Icon, icon, Name, entry.name.c_str(), Size, format_size(entry.size, entry.kind).c_str(),
                               Type, type_name(entry.kind), Modified, modified_text(entry.path).c_str(), Entry, make_entry_tag(static_cast<std::size_t>(&entry - ordered.data())), -1);
            if (icon != nullptr) g_object_unref(icon);
        }
    } else {
        clear_tree_store(tree_store_);
        GtkTreeIter root;
        gtk_tree_store_append(tree_store_, &root, nullptr);
        gtk_tree_store_set(tree_store_, &root, Name, "Current directory", Type, "Folder", Entry, nullptr, -1);
        for (const auto& entry : ordered) {
            GtkTreeIter row;
            gtk_tree_store_append(tree_store_, &row, &root);
            const int icon_size = std::max(16, zoom_level_ * 22 / 100);
            GdkPixbuf* icon = show_icons_ ? entry_icon(entry, icon_size) : nullptr;
            gtk_tree_store_set(tree_store_, &row, Icon, icon, Name, entry.name.c_str(), Size, format_size(entry.size, entry.kind).c_str(),
                               Type, type_name(entry.kind), Modified, modified_text(entry.path).c_str(), Entry, make_entry_tag(static_cast<std::size_t>(&entry - ordered.data())), -1);
            if (icon != nullptr) g_object_unref(icon);
        }
    }
}

void DirectoryView::show_error(const std::string& message) {
    clear_list_store(list_store_);
    clear_tree_store(tree_store_);
    GtkTreeIter row;
    if (list_mode_) {
        gtk_list_store_append(list_store_, &row);
        gtk_list_store_set(list_store_, &row, Name, message.c_str(), Type, "Error", -1);
    } else {
        GtkTreeIter root;
        gtk_tree_store_append(tree_store_, &root, nullptr);
        gtk_tree_store_set(tree_store_, &root, Name, message.c_str(), Type, "Error", -1);
    }
}

FileEntry DirectoryView::selected_entry() const {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_));
    GtkTreeModel* model = nullptr;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) return {};
    FileEntry* entry = nullptr;
    gtk_tree_model_get(model, &iter, Entry, &entry, -1);
    return entry != nullptr ? *entry : FileEntry{};
}

void DirectoryView::select_all_rows() {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_));
    gtk_tree_selection_select_all(selection);
}

void DirectoryView::clear_selection() {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_));
    gtk_tree_selection_unselect_all(selection);
}

void DirectoryView::set_zoom_level(int level) {
    zoom_level_ = std::max(50, std::min(200, level));
    const int icon_size = std::max(16, zoom_level_ * 22 / 100);
    if (icon_renderer_ != nullptr) {
        gtk_cell_renderer_set_fixed_size(GTK_CELL_RENDERER(icon_renderer_), icon_size, icon_size);
    }
    if (name_renderer_ != nullptr) {
        const int font_point_size = std::clamp(zoom_level_ * 10 / 100, 8, 20);
        PangoFontDescription* font = pango_font_description_from_string("Sans");
        pango_font_description_set_size(font, font_point_size * PANGO_SCALE);
        g_object_set(name_renderer_, "font-desc", font, nullptr);
        pango_font_description_free(font);
    }
    if (tree_ != nullptr) {
        gtk_tree_view_columns_autosize(GTK_TREE_VIEW(tree_));
        gtk_widget_queue_resize(tree_);
    }
    if (!entries_cache_.empty()) {
        show_entries(entries_cache_);
    }
}

void DirectoryView::set_show_icons(bool show) {
    show_icons_ = show;
    if (!entries_cache_.empty()) {
        show_entries(entries_cache_);
    }
}

void DirectoryView::set_view_mode(bool list_mode) {
    list_mode_ = list_mode;
    if (tree_ == nullptr) return;
    model_ = list_mode_ ? GTK_TREE_MODEL(list_store_) : GTK_TREE_MODEL(tree_store_);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_), list_mode_);
    if (name_column_ != nullptr) {
        gtk_tree_view_column_set_visible(name_column_, TRUE);
    }
    if (size_column_ != nullptr) {
        gtk_tree_view_column_set_visible(size_column_, list_mode_);
    }
    if (type_column_ != nullptr) {
        gtk_tree_view_column_set_visible(type_column_, list_mode_);
    }
    if (modified_column_ != nullptr) {
        gtk_tree_view_column_set_visible(modified_column_, list_mode_);
    }
    gtk_tree_view_set_model(GTK_TREE_VIEW(tree_), list_mode_ ? GTK_TREE_MODEL(list_store_) : GTK_TREE_MODEL(tree_store_));
    if (!entries_cache_.empty()) {
        show_entries(entries_cache_);
    }
}

}  // namespace turtle::file_manager
