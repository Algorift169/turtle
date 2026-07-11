#include "../include/toolbar.hpp"

#include "../include/icon_loader.hpp"

namespace turtle::file_manager {
namespace {

GtkWidget* tool_button(const char* tooltip, const std::string& icon, const std::function<void()>& action) {
    GtkWidget* button = gtk_button_new(); gtk_widget_set_name(button, "toolbar-button"); gtk_widget_set_tooltip_text(button, tooltip);
    gtk_widget_set_size_request(button, 24, 24);
    gtk_container_add(GTK_CONTAINER(button), IconLoader::image(icon, 18));
    auto* callback = new std::function<void()>(action);
    g_signal_connect_data(button, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer data) { (*static_cast<std::function<void()>*>(data))(); }), callback,
                          +[](gpointer data, GClosure*) { delete static_cast<std::function<void()>*>(data); }, G_CONNECT_AFTER);
    return button;
}

}  // namespace

Toolbar::Toolbar(Actions actions) {
    root_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3); gtk_widget_set_name(root_, "toolbar"); gtk_container_set_border_width(GTK_CONTAINER(root_), 4);
    back_ = tool_button("Back", "images/icons/go-first.png", actions.back);
    forward_ = tool_button("Forward", "images/icons/go-first-rtl.png", actions.forward);
    GtkWidget* up = tool_button("Up Directory", "images/icons/go-up.png", actions.up);
    home_ = tool_button("Home", "images/icons/home.png", actions.home);
    refresh_ = tool_button("Refresh", "images/icons/refresh.svg", actions.refresh);
    path_ = gtk_entry_new(); gtk_editable_set_editable(GTK_EDITABLE(path_), FALSE); gtk_entry_set_placeholder_text(GTK_ENTRY(path_), "Current location");
    GtkWidget* search = gtk_search_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(search), "Search");
    gtk_widget_set_name(path_, "path-entry"); gtk_widget_set_name(search, "search-entry"); gtk_widget_set_size_request(search, 180, -1);
    gtk_box_pack_start(GTK_BOX(root_), back_, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(root_), forward_, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(root_), up, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root_), home_, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(root_), refresh_, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root_), path_, TRUE, TRUE, 4); gtk_box_pack_start(GTK_BOX(root_), search, FALSE, FALSE, 0);
}
GtkWidget* Toolbar::widget() const noexcept { return root_; }
void Toolbar::set_path(const std::string& path) { gtk_entry_set_text(GTK_ENTRY(path_), path.c_str()); }
void Toolbar::set_navigation_state(bool can_go_back, bool can_go_forward) { gtk_widget_set_sensitive(back_, can_go_back); gtk_widget_set_sensitive(forward_, can_go_forward); }

}  // namespace turtle::file_manager
