#include "../include/view_menu.hpp"

#include <utility>

#include "../include/file_manager_window.hpp"

namespace turtle::file_manager {
namespace {

struct MenuActionState {
    FileManagerWindow* window = nullptr;
    void (FileManagerWindow::*callback)() = nullptr;
};

struct SortActionState {
    FileManagerWindow* window = nullptr;
    std::string mode;
};

void invoke_menu_action(GtkMenuItem*, gpointer data) {
    auto* state = static_cast<MenuActionState*>(data);
    if (state == nullptr || state->window == nullptr || state->callback == nullptr) return;
    (state->window->*state->callback)();
}

void invoke_sort_action(GtkMenuItem*, gpointer data) {
    auto* state = static_cast<SortActionState*>(data);
    if (state == nullptr || state->window == nullptr) return;
    state->window->on_view_arrange_by(state->mode);
}

void connect_action(GtkWidget* item, FileManagerWindow* window, void (FileManagerWindow::*callback)()) {
    auto* state = new MenuActionState{window, callback};
    g_signal_connect_data(item, "activate", G_CALLBACK(invoke_menu_action), state,
                          +[](gpointer data, GClosure*) { delete static_cast<MenuActionState*>(data); }, static_cast<GConnectFlags>(0));
}

void connect_sort_action(GtkWidget* item, FileManagerWindow* window, const char* mode) {
    auto* state = new SortActionState{window, mode != nullptr ? mode : ""};
    g_signal_connect_data(item, "activate", G_CALLBACK(invoke_sort_action), state,
                          +[](gpointer data, GClosure*) { delete static_cast<SortActionState*>(data); }, static_cast<GConnectFlags>(0));
}

}  // namespace

void build_view_menu(GtkWidget* menu, FileManagerWindow* window) {
    auto add_item = [&](const char* label, void (FileManagerWindow::*callback)()) {
        GtkWidget* item = gtk_menu_item_new_with_label(label);
        connect_action(item, window, callback);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    };

    add_item("Reload", &FileManagerWindow::on_view_reload);
    add_item("List View", &FileManagerWindow::on_view_list_view);
    add_item("Tree View", &FileManagerWindow::on_view_tree_view);

    GtkWidget* arrange_item = gtk_menu_item_new_with_label("Arrange Items");
    GtkWidget* arrange_menu = gtk_menu_new();
    for (const char* mode : {"by name", "by size", "by type", "by date"}) {
        GtkWidget* sub_item = gtk_menu_item_new_with_label(mode);
        connect_sort_action(sub_item, window, mode);
        gtk_menu_shell_append(GTK_MENU_SHELL(arrange_menu), sub_item);
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(arrange_item), arrange_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), arrange_item);

    add_item("Zoom In", &FileManagerWindow::on_view_zoom_in);
    add_item("Zoom Out", &FileManagerWindow::on_view_zoom_out);
    add_item("Hide Icons", &FileManagerWindow::on_view_hide_icons);
    add_item("Show Icons", &FileManagerWindow::on_view_hide_icons);
}

}  // namespace turtle::file_manager
