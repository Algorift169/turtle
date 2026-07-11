#include "../include/recent_menu.hpp"

#include <utility>

#include "../include/file_manager_window.hpp"

namespace turtle::file_manager {
namespace {

struct RecentActionState {
    FileManagerWindow* window = nullptr;
    std::filesystem::path path;
};

void invoke_recent_action(GtkMenuItem*, gpointer data) {
    auto* state = static_cast<RecentActionState*>(data);
    if (state == nullptr || state->window == nullptr) return;
    state->window->on_recent_open(state->path);
}

void connect_action(GtkWidget* item, FileManagerWindow* window, const std::filesystem::path& path) {
    auto* state = new RecentActionState{window, path};
    g_signal_connect_data(item, "activate", G_CALLBACK(invoke_recent_action), state,
                          +[](gpointer data, GClosure*) { delete static_cast<RecentActionState*>(data); }, static_cast<GConnectFlags>(0));
}

}  // namespace

void build_recent_menu(GtkWidget* menu, FileManagerWindow* window) {
    if (window == nullptr) return;
    const auto items = window->recent_items();
    if (items.empty()) {
        GtkWidget* item = gtk_menu_item_new_with_label("No recent items");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        return;
    }
    for (const auto& path : items) {
        GtkWidget* item = gtk_menu_item_new_with_label(path.string().c_str());
        auto* state = new RecentActionState{window, path};
        g_signal_connect_data(item, "activate", G_CALLBACK(invoke_recent_action), state,
                              +[](gpointer data, GClosure*) { delete static_cast<RecentActionState*>(data); }, static_cast<GConnectFlags>(0));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
}

}  // namespace turtle::file_manager
