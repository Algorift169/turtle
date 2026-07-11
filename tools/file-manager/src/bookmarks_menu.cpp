#include "../include/bookmarks_menu.hpp"

#include <utility>

#include "../include/file_manager_window.hpp"

namespace turtle::file_manager {
namespace {

struct MenuActionState {
    FileManagerWindow* window = nullptr;
    void (FileManagerWindow::*callback)() = nullptr;
};

struct BookmarkActionState {
    FileManagerWindow* window = nullptr;
    std::filesystem::path path;
};

void invoke_menu_action(GtkMenuItem*, gpointer data) {
    auto* state = static_cast<MenuActionState*>(data);
    if (state == nullptr || state->window == nullptr || state->callback == nullptr) return;
    (state->window->*state->callback)();
}

void invoke_bookmark_action(GtkMenuItem*, gpointer data) {
    auto* state = static_cast<BookmarkActionState*>(data);
    if (state == nullptr || state->window == nullptr) return;
    state->window->on_recent_open(state->path);
}

void connect_action(GtkWidget* item, FileManagerWindow* window, void (FileManagerWindow::*callback)()) {
    auto* state = new MenuActionState{window, callback};
    g_signal_connect_data(item, "activate", G_CALLBACK(invoke_menu_action), state,
                          +[](gpointer data, GClosure*) { delete static_cast<MenuActionState*>(data); }, static_cast<GConnectFlags>(0));
}

}  // namespace

void build_bookmarks_menu(GtkWidget* menu, FileManagerWindow* window) {
    GtkWidget* item = gtk_menu_item_new_with_label("Add to Bookmarks");
    connect_action(item, window, &FileManagerWindow::on_bookmarks_add_current);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    for (const auto& bookmark : window->bookmarks()) {
        GtkWidget* bookmark_item = gtk_menu_item_new_with_label(bookmark.string().c_str());
        auto* state = new BookmarkActionState{window, bookmark};
        g_signal_connect_data(bookmark_item, "activate", G_CALLBACK(invoke_bookmark_action), state,
                              +[](gpointer data, GClosure*) { delete static_cast<BookmarkActionState*>(data); }, static_cast<GConnectFlags>(0));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), bookmark_item);
    }
}

}  // namespace turtle::file_manager
