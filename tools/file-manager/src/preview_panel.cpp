#include "preview_panel.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>

namespace turtle::file_manager {
namespace {

constexpr std::size_t kPreviewByteLimit = 64 * 1024;

std::string format_size(std::uintmax_t size) { return std::to_string(size) + " bytes"; }

// Extension matching is useful for icons, but previewing should not hide a
// readable source/configuration file simply because its extension is unknown.
bool read_text_preview(const std::filesystem::path& path, std::string& text) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;

    std::array<char, 4096> buffer{};
    std::size_t total = 0;
    while (input && total < kPreviewByteLimit) {
        const std::size_t requested = std::min(buffer.size(), kPreviewByteLimit - total);
        input.read(buffer.data(), static_cast<std::streamsize>(requested));
        const std::streamsize read = input.gcount();
        if (read <= 0) break;
        if (std::find(buffer.begin(), buffer.begin() + read, '\0') != buffer.begin() + read) return false;
        text.append(buffer.data(), static_cast<std::size_t>(read));
        total += static_cast<std::size_t>(read);
    }
    if (input && total == kPreviewByteLimit) text += "\n\n[Preview limited to 64 KiB]";
    return true;
}

const char* generic_kind(EntryKind kind) {
    switch (kind) {
        case EntryKind::Video: return "Video file";
        case EntryKind::Image: return "Image file";
        case EntryKind::Text: return "Text file";
        default: return "Binary or unknown file";
    }
}

}  // namespace

PreviewPanel::PreviewPanel() {
    root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6); gtk_widget_set_name(root_, "preview-panel"); gtk_container_set_border_width(GTK_CONTAINER(root_), 8); gtk_widget_set_size_request(root_, 210, -1);
    title_ = gtk_label_new("Preview"); gtk_label_set_xalign(GTK_LABEL(title_), 0.0F);
    image_ = gtk_image_new();
    detail_ = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(detail_), FALSE); gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(detail_), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(detail_), GTK_WRAP_WORD_CHAR); gtk_widget_set_name(detail_, "preview-text");
    GtkWidget* detail_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(detail_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(detail_scroll, -1, 210); gtk_container_add(GTK_CONTAINER(detail_scroll), detail_);
    gtk_box_pack_start(GTK_BOX(root_), title_, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(root_), image_, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(root_), detail_scroll, TRUE, TRUE, 0);
}

GtkWidget* PreviewPanel::widget() const noexcept { return root_; }

void PreviewPanel::set_detail(const std::string& text) {
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(detail_));
    gtk_text_buffer_set_text(buffer, text.c_str(), -1);
}

void PreviewPanel::show_message(const char* title, const char* detail) {
    gtk_label_set_text(GTK_LABEL(title_), title); set_detail(detail); gtk_image_clear(GTK_IMAGE(image_));
}

void PreviewPanel::show_entry(const FileEntry& entry) {
    gtk_label_set_text(GTK_LABEL(title_), entry.name.c_str()); gtk_image_clear(GTK_IMAGE(image_));
    if (entry.kind == EntryKind::Directory) {
        show_message(entry.name.c_str(), "Directory\nSingle-click to open this location.");
        return;
    }

    if (entry.kind == EntryKind::Image) {
        GError* error = nullptr;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale(entry.path.c_str(), 180, 140, TRUE, &error);
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_), pixbuf); g_object_unref(pixbuf);
            set_detail("Image file\n" + format_size(entry.size));
        } else {
            set_detail(error ? error->message : "Image preview unavailable.");
            if (error) g_error_free(error);
        }
        return;
    }

    std::string text;
    if (read_text_preview(entry.path, text)) {
        set_detail(text.empty() ? "Empty text file." : text);
        return;
    }

    set_detail(std::string(generic_kind(entry.kind)) + "\n" + format_size(entry.size) +
               "\nA content preview is not available for this binary file type.");
}

}  // namespace turtle::file_manager
