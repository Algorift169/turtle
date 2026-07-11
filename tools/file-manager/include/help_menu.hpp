#pragma once

#include <gtk/gtk.h>

namespace turtle::file_manager {
class FileManagerWindow;

void build_help_menu(GtkWidget* menu, FileManagerWindow* window);

}  // namespace turtle::file_manager
