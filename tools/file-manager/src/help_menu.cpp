#include "../include/help_menu.hpp"

#include "../include/file_manager_window.hpp"

namespace turtle::file_manager {

void build_help_menu(GtkWidget* menu, FileManagerWindow* window) {
    (void)window;
    GtkWidget* item = gtk_menu_item_new_with_label("Help Yourself....!");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}

}  // namespace turtle::file_manager
