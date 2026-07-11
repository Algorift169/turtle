#include "../include/edit_menu.hpp"

#include <utility>

#include "../include/file_manager_window.hpp"

namespace turtle::file_manager {
namespace {

struct MenuActionState {
    FileManagerWindow* window = nullptr;
    void (FileManagerWindow::*callback)() = nullptr;
};

void invoke_menu_action(GtkMenuItem*, gpointer data) {
    auto* state = static_cast<MenuActionState*>(data);
    if (state == nullptr || state->window == nullptr || state->callback == nullptr) return;
    (state->window->*state->callback)();
}

void connect_action(GtkWidget* item, FileManagerWindow* window, void (FileManagerWindow::*callback)()) {
    auto* state = new MenuActionState{window, callback};
    g_signal_connect_data(item, "activate", G_CALLBACK(invoke_menu_action), state,
                          +[](gpointer data, GClosure*) { delete static_cast<MenuActionState*>(data); }, static_cast<GConnectFlags>(0));
}

}  // namespace

void build_edit_menu(GtkWidget* menu, FileManagerWindow* window) {
    auto add_item = [&](const char* label, void (FileManagerWindow::*callback)()) {
        GtkWidget* item = gtk_menu_item_new_with_label(label);
        connect_action(item, window, callback);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    };

    add_item("Cut", &FileManagerWindow::on_edit_cut);
    add_item("Move to Trash", &FileManagerWindow::on_edit_move_to_trash);
    add_item("Select All Files", &FileManagerWindow::on_edit_select_all);
    add_item("Unselect All Files", &FileManagerWindow::on_edit_unselect_all);
    add_item("Rename", &FileManagerWindow::on_edit_rename);
    add_item("Change Location", &FileManagerWindow::on_edit_change_location);
}

}  // namespace turtle::file_manager
