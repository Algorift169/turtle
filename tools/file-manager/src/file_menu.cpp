#include "../include/file_menu.hpp"

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

void build_file_menu(GtkWidget* menu, FileManagerWindow* window) {
    auto add_item = [&](const char* label, void (FileManagerWindow::*callback)()) {
        GtkWidget* item = gtk_menu_item_new_with_label(label);
        connect_action(item, window, callback);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    };

    add_item("Copy", &FileManagerWindow::on_file_copy);
    add_item("Paste", &FileManagerWindow::on_file_paste);
    add_item("New Window", &FileManagerWindow::on_file_new_window);
    add_item("Create Folder", &FileManagerWindow::on_file_create_folder);
    add_item("Create Document", &FileManagerWindow::on_file_create_document);
    add_item("Properties", &FileManagerWindow::on_file_properties);
    add_item("Close Window", &FileManagerWindow::on_file_close_window);
    add_item("Close All Windows", &FileManagerWindow::on_file_close_all_windows);
}

}  // namespace turtle::file_manager
