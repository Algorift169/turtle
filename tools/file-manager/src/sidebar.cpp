#include "../include/sidebar.hpp"

#include <vector>

#include "../include/file_system.hpp"
#include "../include/icon_loader.hpp"

namespace turtle::file_manager {
namespace {

struct Location { const char* label; const char* icon; std::filesystem::path path; };
struct LocationCallback { Sidebar* sidebar; Sidebar::LocationRequested callback; std::filesystem::path path; GtkWidget* row; };

GtkWidget* location_row(const Location& location, Sidebar* sidebar, const Sidebar::LocationRequested& callback) {
    GtkWidget* button = gtk_button_new();
    gtk_widget_set_name(button, "sidebar-row");
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(row), 3);
    gtk_box_pack_start(GTK_BOX(row), IconLoader::image(location.icon, 18, "folder"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), gtk_label_new(location.label), TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(button), row);
    g_signal_connect_data(button, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer data) {
        auto* request = static_cast<LocationCallback*>(data);
        request->sidebar->select_row(request->row);
        request->callback(request->path);
    }), new LocationCallback{sidebar, callback, location.path, button},
    +[](gpointer data, GClosure*) { delete static_cast<LocationCallback*>(data); }, G_CONNECT_AFTER);
    return button;
}

}  // namespace

Sidebar::Sidebar(LocationRequested on_location_requested) {
    root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(root_, 145, -1);
    const auto home = FileSystem::home_directory();
    const std::vector<Location> locations = {
        {"Desktop", "images/icons/desktop.svg", home / "Desktop"}, {"Home", "images/icons/home.png", home},
        {"Downloads", "images/icons/go-down.png", home / "Downloads"}, {"Documents", "images/icons/folder.svg", home / "Documents"},
        {"Pictures", "images/icons/folder.svg", home / "Pictures"}, {"Music", "images/icons/folder.svg", home / "Music"},
        {"Videos", "images/icons/folder.svg", home / "Videos"}, {"Trash", "images/icons/user-trash-full.png", FileSystem::trash_directory()},
        {"Recent", "images/icons/refresh.svg", home}
    };
    for (const auto& location : locations) {
        GtkWidget* row = location_row(location, this, on_location_requested);
        gtk_box_pack_start(GTK_BOX(root_), row, FALSE, FALSE, 0);
        if (std::string(location.label) == "Home") select_row(row);
    }
}

GtkWidget* Sidebar::widget() const noexcept { return root_; }

void Sidebar::select_row(GtkWidget* row) {
    if (selected_row_) gtk_style_context_remove_class(gtk_widget_get_style_context(selected_row_), "selected");
    selected_row_ = row;
    gtk_style_context_add_class(gtk_widget_get_style_context(selected_row_), "selected");
}

}  // namespace turtle::file_manager
