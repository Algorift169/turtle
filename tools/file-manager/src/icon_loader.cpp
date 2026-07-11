#include "icon_loader.hpp"

#include <filesystem>

namespace turtle::file_manager {

GtkWidget* IconLoader::image(const std::string& project_icon, int pixel_size, const char* fallback_icon) {
    if (std::filesystem::exists(project_icon)) {
        GError* error = nullptr;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale(project_icon.c_str(), pixel_size, pixel_size, TRUE, &error);
        if (pixbuf != nullptr) {
            GtkWidget* result = gtk_image_new_from_pixbuf(pixbuf);
            g_object_unref(pixbuf);
            return result;
        }
        if (error != nullptr) g_error_free(error);
    }
    GtkWidget* result = gtk_image_new_from_icon_name(fallback_icon, GTK_ICON_SIZE_BUTTON);
    gtk_image_set_pixel_size(GTK_IMAGE(result), pixel_size);
    return result;
}

GtkWidget* IconLoader::entry_icon(bool is_directory, int pixel_size) {
    return is_directory ? image("images/icons/folder.svg", pixel_size, "folder")
                        : image("", pixel_size, "text-x-generic");
}

}  // namespace turtle::file_manager
