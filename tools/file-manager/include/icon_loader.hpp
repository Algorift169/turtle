#pragma once

#include <gtk/gtk.h>
#include <string>

namespace turtle::file_manager {

// Centralising asset lookup keeps project artwork preferred while allowing a
// safe GTK fallback when an optional asset is unavailable during development.
class IconLoader {
public:
    static GtkWidget* image(const std::string& project_icon, int pixel_size,
                            const char* fallback_icon = "text-x-generic");
    static GtkWidget* entry_icon(bool is_directory, int pixel_size);
};

}  // namespace turtle::file_manager
