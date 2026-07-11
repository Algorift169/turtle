#include "../include/file_manager_window.hpp"

#include <string>

#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../include/bookmarks_menu.hpp"
#include "../include/edit_menu.hpp"
#include "../include/file_menu.hpp"
#include "../include/help_menu.hpp"
#include "../include/icon_loader.hpp"
#include "../include/recent_menu.hpp"
#include "../include/view_menu.hpp"

namespace turtle::file_manager {
namespace {

void add_menu(GtkWidget* bar, const char* name, GtkWidget* menu) {
    GtkWidget* item = gtk_menu_item_new_with_label(name);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), item);
}

namespace {

constexpr const char* kAppIdProperty = "_TURTLE_APP_ID";
constexpr const char* kAppIconProperty = "_TURTLE_APP_ICON";
constexpr const char* kAppRegisterAtom = "_TURTLE_APP_REGISTER";
constexpr const char* kAppRestoreAtom = "_TURTLE_APP_RESTORE";

std::string prompt_for_text(const char* title, const char* hint) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(nullptr), GTK_DIALOG_MODAL,
                                                    "Cancel", GTK_RESPONSE_CANCEL,
                                                    "OK", GTK_RESPONSE_OK, nullptr);
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), hint);
    gtk_box_pack_start(GTK_BOX(content_area), entry, TRUE, TRUE, 6);
    gtk_widget_show_all(dialog);
    const gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    std::string result;
    if (response == GTK_RESPONSE_OK) {
        result = gtk_entry_get_text(GTK_ENTRY(entry));
    }
    gtk_widget_destroy(dialog);
    return result;
}

std::pair<std::string, std::string> prompt_for_rename(const char* current_name) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Rename", GTK_WINDOW(nullptr), GTK_DIALOG_MODAL,
                                                    "Cancel", GTK_RESPONSE_CANCEL,
                                                    "OK", GTK_RESPONSE_OK, nullptr);
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget* current_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(current_entry), current_name);
    gtk_entry_set_placeholder_text(GTK_ENTRY(current_entry), "Current name");
    gtk_widget_set_sensitive(current_entry, FALSE);

    GtkWidget* new_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(new_entry), "New name");

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(box), current_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), new_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), box, TRUE, TRUE, 6);

    gtk_widget_show_all(dialog);
    const gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    std::pair<std::string, std::string> result{"", ""};
    if (response == GTK_RESPONSE_OK) {
        result.first = gtk_entry_get_text(GTK_ENTRY(current_entry));
        result.second = gtk_entry_get_text(GTK_ENTRY(new_entry));
    }
    gtk_widget_destroy(dialog);
    return result;
}

void show_message(const char* title, const std::string& message) {
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(nullptr), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK, "%s", title);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", message.c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void set_string_property(GtkWidget* widget, const char* property_name, const std::string& value) {
    if (widget == nullptr || value.empty()) {
        return;
    }
    auto* gdk_window = gtk_widget_get_window(widget);
    if (gdk_window == nullptr) {
        return;
    }
    Display* display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    Window x_window = GDK_WINDOW_XID(gdk_window);
    Atom atom = XInternAtom(display, property_name, False);
    if (atom == None) {
        return;
    }
    XChangeProperty(display, x_window, atom, XA_STRING, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(value.c_str()), static_cast<int>(value.size() + 1));
}

void send_window_message(GtkWidget* widget, const char* message_name) {
    auto* gdk_window = gtk_widget_get_window(widget);
    if (gdk_window == nullptr) {
        return;
    }
    Display* display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    Window x_window = GDK_WINDOW_XID(gdk_window);
    XClientMessageEvent event{};
    event.type = ClientMessage;
    event.window = DefaultRootWindow(display);
    event.message_type = XInternAtom(display, message_name, False);
    event.format = 32;
    event.data.l[0] = 1;
    event.data.l[1] = static_cast<long>(x_window);
    XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, reinterpret_cast<XEvent*>(&event));
    XFlush(display);
}

GdkFilterReturn window_event_filter(GdkXEvent* xevent, GdkEvent*, gpointer data) {
    auto* event = static_cast<XEvent*>(xevent);
    if (event->type != ClientMessage) {
        return GDK_FILTER_CONTINUE;
    }

    auto* widget = static_cast<GtkWidget*>(data);
    if (widget == nullptr) {
        return GDK_FILTER_CONTINUE;
    }

    Display* display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    Atom restore_atom = XInternAtom(display, kAppRestoreAtom, False);
    if (event->xclient.message_type != restore_atom) {
        return GDK_FILTER_CONTINUE;
    }

    gtk_window_deiconify(GTK_WINDOW(widget));
    gtk_window_present(GTK_WINDOW(widget));
    return GDK_FILTER_REMOVE;
}

}  // namespace

GtkWidget* circular_window_button(const char* css_class, const char* tooltip, GCallback callback, gpointer owner) {
    GtkWidget* button = gtk_button_new(); gtk_widget_set_name(button, css_class); gtk_widget_set_tooltip_text(button, tooltip);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    gtk_widget_set_can_focus(button, FALSE);
    // Keep the controls square and prevent the title bar from expanding them.
    gtk_widget_set_size_request(button, 16, 16);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(button, FALSE);
    gtk_widget_set_vexpand(button, FALSE);
    gtk_widget_set_margin_start(button, 0);
    gtk_widget_set_margin_end(button, 0);
    gtk_widget_set_margin_top(button, 0);
    gtk_widget_set_margin_bottom(button, 0);
    g_signal_connect(button, "realize", G_CALLBACK(+[](GtkWidget* widget, gpointer) {
        GdkWindow* window = gtk_widget_get_window(widget);
        if (window == nullptr) return;
        GdkCursor* cursor = gdk_cursor_new_from_name(gdk_window_get_display(window), "pointer");
        if (cursor != nullptr) {
            gdk_window_set_cursor(window, cursor);
            g_object_unref(cursor);
        }
    }), nullptr);
    g_signal_connect(button, "clicked", callback, owner);
    return button;
}

}  // namespace

FileManagerWindow::FileManagerWindow() : navigation_(FileSystem::home_directory()) {
    window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window_), 860, 520);
    gtk_window_set_title(GTK_WINDOW(window_), "Turtle File-System");
    gtk_window_set_titlebar(GTK_WINDOW(window_), create_title_bar());
    gtk_widget_realize(window_);
    set_string_property(window_, kAppIdProperty, "turtle-file-manager");
    set_string_property(window_, kAppIconProperty, "images/icons/file-manager.svg");
    gdk_window_add_filter(gtk_widget_get_window(window_), window_event_filter, window_);
    g_signal_connect(window_, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    auto* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background: #1A1A1A; color: #e2e2e2; } "
        "headerbar, headerbar#compact-titlebar { background: #1A1A1A; border-bottom: 1px solid #2D2D2D; min-height: 24px; padding: 0; margin: 0; } "
        "headerbar#compact-titlebar button { min-width: 0; min-height: 0; padding: 0; margin: 0; } "
        "menubar, #toolbar, #status-bar { background: #232323; border-color: #2D2D2D; } "
        "menubar { padding: 0; border-bottom: 1px solid #2D2D2D; } menubar > menuitem { padding: 3px 7px; } "
        "#toolbar { border-bottom: 1px solid #2D2D2D; } #toolbar-button { min-width: 24px; min-height: 24px; padding: 1px; border-radius: 2px; } "
        "#toolbar-button:hover, #sidebar-row:hover { background: #2D2D2D; } "
        "entry, searchentry { background: #1A1A1A; color: #eeeeee; border: 1px solid #2D2D2D; border-radius: 2px; padding: 3px 5px; } "
        "#sidebar-row { background: transparent; border: 0; border-radius: 0; padding: 0; } #sidebar-row.selected { background: #315843; } "
        "treeview.view { background: #1A1A1A; color: #e2e2e2; } treeview.view:selected { background: #315843; color: #ffffff; } "
        "treeview header button { background: #232323; color: #e2e2e2; border-color: #2D2D2D; padding: 3px 6px; } "
        "paned > separator { background: #2D2D2D; min-width: 1px; } #preview-panel { background: #232323; } "
        "#status-bar { border-top: 1px solid #2D2D2D; padding: 3px 7px; color: #bdbdbd; } "
        "#window-controls { padding: 0; margin: 0; } "
        "#yellow, #green, #red { min-width: 16px; min-height: 16px; padding: 0; margin: 0; border: 0; border-radius: 9999px; box-shadow: none; background-image: none; } "
        "#yellow { background-color: #F4BF4F; } #yellow:hover { background-color: #F8CC69; } #yellow:active { background-color: #D9A43E; } "
        "#green { background-color: #61C554; } #green:hover { background-color: #78D46D; } #green:active { background-color: #50A646; } "
        "#red { background-color: #E35D5B; } #red:hover { background-color: #EA7774; } #red:active { background-color: #C74E4C; }", -1, nullptr);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION); g_object_unref(provider);

    root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_container_add(GTK_CONTAINER(window_), root_);
    menu_bar_ = create_menu_bar();
    gtk_box_pack_start(GTK_BOX(root_), menu_bar_, FALSE, FALSE, 0);
    toolbar_ = std::make_unique<Toolbar>(Toolbar::Actions{
        [this] { navigation_.go_back(); }, [this] { navigation_.go_forward(); }, [this] { navigation_.go_up(); },
        [this] { navigate_to(FileSystem::home_directory()); }, [this] { refresh_directory(navigation_.current_location()); }});
    gtk_box_pack_start(GTK_BOX(root_), toolbar_->widget(), FALSE, FALSE, 0);
    GtkWidget* panes = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); gtk_box_pack_start(GTK_BOX(root_), panes, TRUE, TRUE, 0);
    sidebar_ = std::make_unique<Sidebar>([this](const std::filesystem::path& path) { navigate_to(path); }); gtk_paned_pack1(GTK_PANED(panes), sidebar_->widget(), FALSE, FALSE); gtk_paned_set_position(GTK_PANED(panes), 145);
    GtkWidget* right_panes = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); gtk_paned_pack2(GTK_PANED(panes), right_panes, TRUE, FALSE);
    // Apply a 60/40 list/preview ratio only once; after that the user can
    // continue to adjust the divider normally.
    g_signal_connect(right_panes, "size-allocate", G_CALLBACK(+[](GtkWidget* widget, GtkAllocation* allocation, gpointer) {
        if (allocation->width <= 0 || GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "initial-split-applied"))) return;
        gtk_paned_set_position(GTK_PANED(widget), allocation->width * 3 / 5);
        g_object_set_data(G_OBJECT(widget), "initial-split-applied", GINT_TO_POINTER(1));
    }), nullptr);
    directory_view_ = std::make_unique<DirectoryView>([this](const FileEntry& entry) { activate_entry(entry); }, [this](const FileEntry& entry) { select_entry(entry); });
    GtkWidget* content_scroll = gtk_scrolled_window_new(nullptr, nullptr); gtk_container_add(GTK_CONTAINER(content_scroll), directory_view_->widget()); gtk_paned_pack1(GTK_PANED(right_panes), content_scroll, TRUE, FALSE);
    preview_panel_ = std::make_unique<PreviewPanel>(); GtkWidget* preview_scroll = gtk_scrolled_window_new(nullptr, nullptr); gtk_container_add(GTK_CONTAINER(preview_scroll), preview_panel_->widget()); gtk_paned_pack2(GTK_PANED(right_panes), preview_scroll, FALSE, FALSE);
    status_ = gtk_label_new(""); gtk_widget_set_name(status_, "status-bar"); gtk_label_set_xalign(GTK_LABEL(status_), 0.0F); gtk_box_pack_end(GTK_BOX(root_), status_, FALSE, FALSE, 0);
    navigation_.set_location_changed_handler([this](const std::filesystem::path& path) { refresh_directory(path); });
    load_bookmarks();
    load_recent();
    refresh_directory(navigation_.current_location());
}

void FileManagerWindow::show() {
    gtk_window_unfullscreen(GTK_WINDOW(window_));
    gtk_window_unmaximize(GTK_WINDOW(window_));
    gtk_widget_show_all(window_);
}

void FileManagerWindow::navigate_to(const std::filesystem::path& path) {
    std::string error; if (!file_system_.is_directory_readable(path, error)) { preview_panel_->show_message("Cannot open location", error.c_str()); return; }
    if (!navigation_.navigate_to(path)) refresh_directory(path);
}

void FileManagerWindow::refresh_directory(const std::filesystem::path& path) {
    std::string error; auto entries = file_system_.read_directory(path, error); toolbar_->set_path(path.string()); toolbar_->set_navigation_state(navigation_.can_go_back(), navigation_.can_go_forward());
    if (!error.empty()) { directory_view_->show_error("Unable to display " + path.string() + ": " + error); preview_panel_->show_message("Directory unavailable", error.c_str()); set_status(error); return; }
    if (view_sort_mode_ == "by size") {
        std::sort(entries.begin(), entries.end(), [](const FileEntry& left, const FileEntry& right) { return left.size < right.size; });
    } else if (view_sort_mode_ == "by type") {
        std::sort(entries.begin(), entries.end(), [](const FileEntry& left, const FileEntry& right) { return left.kind < right.kind; });
    } else if (view_sort_mode_ == "by date") {
        std::sort(entries.begin(), entries.end(), [](const FileEntry& left, const FileEntry& right) { return left.path < right.path; });
    } else {
        std::sort(entries.begin(), entries.end(), [](const FileEntry& left, const FileEntry& right) { return left.name < right.name; });
    }
    directory_view_->set_show_icons(!hide_icons_);
    directory_view_->set_view_mode(list_view_);
    directory_view_->show_entries(entries); preview_panel_->show_message("Preview", "Select an item to inspect it.");
    set_status(std::to_string(entries.size()) + " items  |  " + path.string());
}

void FileManagerWindow::activate_entry(const FileEntry& entry) {
    mark_recent(entry.path);
    if (entry.kind == EntryKind::Directory) navigate_to(entry.path);
}

void FileManagerWindow::select_entry(const FileEntry& entry) {
    mark_recent(entry.path);
    preview_panel_->show_entry(entry);
}

void FileManagerWindow::mark_recent(const std::filesystem::path& path) {
    if (path.empty()) return;
    ++recent_counts_[path];
    save_recent();
    refresh_menu_bar();
}

void FileManagerWindow::load_bookmarks() {
    std::ifstream input(std::filesystem::path(FileSystem::home_directory() / ".turtle_file_manager_bookmarks"));
    if (!input) return;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) bookmarks_.push_back(std::filesystem::path(line));
    }
}

void FileManagerWindow::save_bookmarks() {
    std::ofstream output(std::filesystem::path(FileSystem::home_directory() / ".turtle_file_manager_bookmarks"));
    for (const auto& path : bookmarks_) output << path.string() << '\n';
}

void FileManagerWindow::load_recent() {
    std::ifstream input(std::filesystem::path(FileSystem::home_directory() / ".turtle_file_manager_recent"));
    if (!input) return;
    std::string line; int count = 0;
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        std::stringstream stream(line);
        std::string path; int usage = 0;
        if (std::getline(stream, path, ':') && stream >> usage) recent_counts_[std::filesystem::path(path)] = usage;
    }
    (void)count;
}

void FileManagerWindow::save_recent() {
    std::ofstream output(std::filesystem::path(FileSystem::home_directory() / ".turtle_file_manager_recent"));
    for (const auto& [path, count] : recent_counts_) output << path.string() << ':' << count << '\n';
}

void FileManagerWindow::refresh_menu_bar() {
    if (menu_bar_ == nullptr) {
        menu_bar_ = gtk_menu_bar_new();
        if (root_ != nullptr) {
            gtk_box_pack_start(GTK_BOX(root_), menu_bar_, FALSE, FALSE, 0);
        }
    } else {
        GList* children = gtk_container_get_children(GTK_CONTAINER(menu_bar_));
        for (GList* iter = children; iter != nullptr; iter = iter->next) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);
    }

    GtkWidget* file_menu = gtk_menu_new();
    build_file_menu(file_menu, this);
    add_menu(menu_bar_, "File", file_menu);

    GtkWidget* edit_menu = gtk_menu_new();
    build_edit_menu(edit_menu, this);
    add_menu(menu_bar_, "Edit", edit_menu);

    GtkWidget* view_menu = gtk_menu_new();
    build_view_menu(view_menu, this);
    add_menu(menu_bar_, "View", view_menu);

    GtkWidget* bookmarks_menu = gtk_menu_new();
    build_bookmarks_menu(bookmarks_menu, this);
    add_menu(menu_bar_, "Bookmarks", bookmarks_menu);

    GtkWidget* recent_menu = gtk_menu_new();
    build_recent_menu(recent_menu, this);
    add_menu(menu_bar_, "Recent", recent_menu);

    GtkWidget* help_menu = gtk_menu_new();
    build_help_menu(help_menu, this);
    add_menu(menu_bar_, "Help", help_menu);

    gtk_widget_show_all(menu_bar_);
}

std::filesystem::path FileManagerWindow::current_location() const { return navigation_.current_location(); }
std::vector<FileEntry> FileManagerWindow::selected_entries() const { return {}; }
FileEntry FileManagerWindow::selected_entry() const { return directory_view_ ? directory_view_->selected_entry() : FileEntry{}; }
std::vector<std::filesystem::path> FileManagerWindow::bookmarks() const { return bookmarks_; }
std::vector<std::filesystem::path> FileManagerWindow::recent_items() const {
    std::vector<std::pair<std::filesystem::path, int>> entries(recent_counts_.begin(), recent_counts_.end());
    std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) { return left.second > right.second; });
    std::vector<std::filesystem::path> result; result.reserve(entries.size());
    for (const auto& [path, _] : entries) result.push_back(path);
    return result;
}

void FileManagerWindow::on_file_copy() {
    const auto entry = selected_entry();
    if (entry.path.empty()) { set_status("Select a file or folder to copy"); return; }
    clipboard_items_ = {entry.path}; clipboard_mode_ = ClipboardMode::Copy; set_status("Copied " + entry.name);
}

void FileManagerWindow::on_file_paste() {
    if (clipboard_items_.empty()) { set_status("Nothing to paste"); return; }
    const auto destination_dir = current_location();
    bool moved = false;
    for (const auto& item : clipboard_items_) {
        const auto target = destination_dir / item.filename();
        std::filesystem::path final_target = target;
        std::size_t suffix = 1;
        while (std::filesystem::exists(final_target)) {
            std::ostringstream name; name << item.stem().string() << "-copy" << suffix++ << item.extension().string();
            final_target = destination_dir / name.str();
        }
        if (clipboard_mode_ == ClipboardMode::Cut) {
            std::error_code ec; std::filesystem::rename(item, final_target, ec);
            if (ec) {
                std::filesystem::copy(item, final_target, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                std::filesystem::remove_all(item);
            }
            moved = true;
        } else {
            std::filesystem::copy(item, final_target, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
        }
    }
    clipboard_items_.clear(); clipboard_mode_ = ClipboardMode::NoneMode;
    refresh_directory(current_location());
    set_status(moved ? "Moved clipboard contents" : "Pasted clipboard contents");
}

void FileManagerWindow::on_file_new_window() {
    std::string command = "./build/bin/turtle-file-manager &";
    std::system(command.c_str());
    set_status("Opened a new file manager window");
}

void FileManagerWindow::on_file_create_folder() {
    const auto name = prompt_for_text("Create Folder", "Folder name");
    if (name.empty()) return;
    const auto path = current_location() / name;
    std::error_code ec; std::filesystem::create_directory(path, ec);
    if (ec) { set_status("Could not create folder: " + ec.message()); return; }
    refresh_directory(current_location());
    set_status("Created folder " + name);
}

void FileManagerWindow::on_file_create_document() {
    const auto name = prompt_for_text("Create Document", "Document name");
    if (name.empty()) return;
    const auto path = current_location() / (name.find('.') != std::string::npos ? name : name + ".txt");
    std::ofstream output(path);
    output.close();
    refresh_directory(current_location());
    set_status("Created document " + path.filename().string());
}

void FileManagerWindow::on_file_properties() {
    const auto entry = selected_entry();
    const auto target = entry.path.empty() ? current_location() : entry.path;
    std::ostringstream details;
    details << "Name: " << target.filename().string() << "\n";
    details << "Path: " << target.string() << "\n";
    details << "Type: " << (std::filesystem::is_directory(target) ? "Directory" : "File") << "\n";
    if (std::filesystem::exists(target)) {
        std::error_code ec; const auto size = std::filesystem::file_size(target, ec); details << "Size: " << (ec ? "unknown" : std::to_string(size)) << " bytes\n";
        const auto stamp = std::filesystem::last_write_time(target, ec); if (!ec) {
            const auto system_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                stamp - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            const auto time_t_value = std::chrono::system_clock::to_time_t(system_time);
            details << "Modified: " << std::ctime(const_cast<std::time_t*>(&time_t_value));
        }
    }
    show_message("Properties", details.str());
}

void FileManagerWindow::on_file_close_window() { gtk_widget_destroy(window_); }
void FileManagerWindow::on_file_close_all_windows() { gtk_main_quit(); }
void FileManagerWindow::on_edit_cut() {
    const auto entry = selected_entry();
    if (entry.path.empty()) { set_status("Select a file or folder to cut"); return; }
    clipboard_items_ = {entry.path}; clipboard_mode_ = ClipboardMode::Cut; set_status("Cut " + entry.name);
}

void FileManagerWindow::on_edit_move_to_trash() {
    const auto entry = selected_entry();
    if (entry.path.empty()) { set_status("Select a file or folder to move to trash"); return; }
    const auto trash_dir = FileSystem::home_directory() / ".local/share/Trash/files";
    std::filesystem::create_directories(trash_dir);
    std::filesystem::path target = trash_dir / entry.path.filename();
    std::size_t suffix = 1; while (std::filesystem::exists(target)) { std::ostringstream name; name << entry.path.stem().string() << "-" << suffix++ << entry.path.extension().string(); target = trash_dir / name.str(); }
    std::filesystem::rename(entry.path, target);
    refresh_directory(current_location());
    set_status("Moved to trash");
}

void FileManagerWindow::on_edit_select_all() { directory_view_->select_all_rows(); }
void FileManagerWindow::on_edit_unselect_all() { directory_view_->clear_selection(); }
void FileManagerWindow::on_edit_rename() {
    const auto entry = selected_entry();
    if (entry.path.empty()) { set_status("Select a file or folder to rename"); return; }
    const auto [current_name, new_name] = prompt_for_rename(entry.path.filename().string().c_str());
    if (new_name.empty()) return;
    const auto target = entry.path.parent_path() / new_name;
    if (target == entry.path) { set_status("New name matches current name"); return; }
    std::error_code ec; std::filesystem::rename(entry.path, target, ec);
    if (ec) { set_status("Rename failed: " + ec.message()); return; }
    refresh_directory(current_location());
    set_status("Renamed " + current_name + " to " + new_name);
}

void FileManagerWindow::on_edit_change_location() {
    const auto target = prompt_for_text("Change Location", "Path");
    if (target.empty()) return;
    navigate_to(std::filesystem::path(target));
}

void FileManagerWindow::on_view_reload() { refresh_directory(current_location()); }
void FileManagerWindow::on_view_list_view() { list_view_ = true; refresh_directory(current_location()); }
void FileManagerWindow::on_view_tree_view() { list_view_ = false; refresh_directory(current_location()); }
void FileManagerWindow::on_view_arrange_by(const std::string& mode) { view_sort_mode_ = mode; refresh_directory(current_location()); }
void FileManagerWindow::on_view_zoom_in() { directory_view_->set_zoom_level(120); set_status("Zoom in"); }
void FileManagerWindow::on_view_zoom_out() { directory_view_->set_zoom_level(80); set_status("Zoom out"); }
void FileManagerWindow::on_view_hide_icons() { hide_icons_ = !hide_icons_; directory_view_->set_show_icons(!hide_icons_); refresh_directory(current_location()); }
void FileManagerWindow::on_bookmarks_add_current() { bookmarks_.push_back(current_location()); save_bookmarks(); refresh_menu_bar(); set_status("Added current location to bookmarks"); }
void FileManagerWindow::on_recent_open(const std::filesystem::path& path) { if (!path.empty()) navigate_to(path); }
void FileManagerWindow::on_help() { set_status("Help Yourself....!"); }

void FileManagerWindow::set_status(const std::string& message) { gtk_label_set_text(GTK_LABEL(status_), message.c_str()); }

GtkWidget* FileManagerWindow::create_menu_bar() {
    GtkWidget* bar = gtk_menu_bar_new();
    GtkWidget* file_menu = gtk_menu_new(); build_file_menu(file_menu, this); add_menu(bar, "File", file_menu);
    GtkWidget* edit_menu = gtk_menu_new(); build_edit_menu(edit_menu, this); add_menu(bar, "Edit", edit_menu);
    GtkWidget* view_menu = gtk_menu_new(); build_view_menu(view_menu, this); add_menu(bar, "View", view_menu);
    GtkWidget* bookmarks_menu = gtk_menu_new(); build_bookmarks_menu(bookmarks_menu, this); add_menu(bar, "Bookmarks", bookmarks_menu);
    GtkWidget* recent_menu = gtk_menu_new(); build_recent_menu(recent_menu, this); add_menu(bar, "Recent", recent_menu);
    GtkWidget* help_menu = gtk_menu_new(); build_help_menu(help_menu, this); add_menu(bar, "Help", help_menu);
    return bar;
}
GtkWidget* FileManagerWindow::create_title_bar() {
    GtkWidget* header = gtk_header_bar_new(); gtk_widget_set_name(header, "compact-titlebar"); gtk_widget_set_size_request(header, -1, 24);
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), FALSE); gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Turtle File-System");
    GtkWidget* controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_name(controls, "window-controls");
    gtk_widget_set_halign(controls, GTK_ALIGN_END);
    gtk_widget_set_valign(controls, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(controls, FALSE);
    gtk_widget_set_vexpand(controls, FALSE);
    gtk_widget_set_size_request(controls, -1, 24);
    gtk_box_pack_start(GTK_BOX(controls), circular_window_button("yellow", "Minimize", G_CALLBACK(+[](GtkWidget*, gpointer data) {
        auto* window = GTK_WINDOW(data);
        gtk_window_iconify(window);
        send_window_message(GTK_WIDGET(window), kAppRegisterAtom);
    }), window_), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls), circular_window_button("green", "Toggle Fullscreen", G_CALLBACK(+[](GtkWidget*, gpointer data) {
        auto* window = GTK_WINDOW(data);
        // GTK fullscreen intentionally hides client title bars. Maximize gives
        // Turtle the same desktop-sized workspace while keeping all three
        // application window controls available.
        const bool maximized = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), "turtle-maximized"));
        if (maximized) gtk_window_unmaximize(window); else gtk_window_maximize(window);
        g_object_set_data(G_OBJECT(window), "turtle-maximized", GINT_TO_POINTER(!maximized));
    }), window_), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls), circular_window_button("red", "Close", G_CALLBACK(+[](GtkWidget*, gpointer data) { gtk_widget_destroy(GTK_WIDGET(data)); }), window_), FALSE, FALSE, 0);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), controls); return header;
}

}  // namespace turtle::file_manager
