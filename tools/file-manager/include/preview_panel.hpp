#pragma once

#include <string>

#include <gtk/gtk.h>

#include "file_system.hpp"

namespace turtle::file_manager {

class PreviewPanel {
public:
    PreviewPanel();
    GtkWidget* widget() const noexcept;
    void show_entry(const FileEntry& entry);
    void show_message(const char* title, const char* detail);

private:
    void set_detail(const std::string& text);
    GtkWidget* root_ = nullptr;
    GtkWidget* title_ = nullptr;
    GtkWidget* detail_ = nullptr;
    GtkWidget* image_ = nullptr;
};

}  // namespace turtle::file_manager
