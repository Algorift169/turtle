#include "../include/file_manager_window.hpp"

#include <string>

#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "../include/icon_loader.hpp"

namespace turtle::file_manager {
namespace {

void add_menu(GtkWidget* bar, const char* name) {
    GtkWidget* item = gtk_menu_item_new_with_label(name); GtkWidget* menu = gtk_menu_new();
    GtkWidget* placeholder = gtk_menu_item_new_with_label("Action placeholder"); gtk_menu_shell_append(GTK_MENU_SHELL(menu), placeholder);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu); gtk_menu_shell_append(GTK_MENU_SHELL(bar), item);
}

namespace {

constexpr const char* kAppIdProperty = "_TURTLE_APP_ID";
constexpr const char* kAppIconProperty = "_TURTLE_APP_ICON";
constexpr const char* kAppRegisterAtom = "_TURTLE_APP_REGISTER";
constexpr const char* kAppRestoreAtom = "_TURTLE_APP_RESTORE";

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

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_container_add(GTK_CONTAINER(window_), root);
    gtk_box_pack_start(GTK_BOX(root), create_menu_bar(), FALSE, FALSE, 0);
    toolbar_ = std::make_unique<Toolbar>(Toolbar::Actions{
        [this] { navigation_.go_back(); }, [this] { navigation_.go_forward(); }, [this] { navigation_.go_up(); },
        [this] { navigate_to(FileSystem::home_directory()); }, [this] { refresh_directory(navigation_.current_location()); }});
    gtk_box_pack_start(GTK_BOX(root), toolbar_->widget(), FALSE, FALSE, 0);
    GtkWidget* panes = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); gtk_box_pack_start(GTK_BOX(root), panes, TRUE, TRUE, 0);
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
    status_ = gtk_label_new(""); gtk_widget_set_name(status_, "status-bar"); gtk_label_set_xalign(GTK_LABEL(status_), 0.0F); gtk_box_pack_end(GTK_BOX(root), status_, FALSE, FALSE, 0);
    navigation_.set_location_changed_handler([this](const std::filesystem::path& path) { refresh_directory(path); });
    refresh_directory(navigation_.current_location());
}

void FileManagerWindow::show() {
    // Start each instance as a normal desktop window, even when the window
    // manager previously supplied a maximized or fullscreen state.
    gtk_window_unfullscreen(GTK_WINDOW(window_));
    gtk_window_unmaximize(GTK_WINDOW(window_));
    gtk_widget_show_all(window_);
}
void FileManagerWindow::navigate_to(const std::filesystem::path& path) {
    std::string error; if (!file_system_.is_directory_readable(path, error)) { preview_panel_->show_message("Cannot open location", error.c_str()); return; }
    if (!navigation_.navigate_to(path)) refresh_directory(path);
}
void FileManagerWindow::refresh_directory(const std::filesystem::path& path) {
    std::string error; const auto entries = file_system_.read_directory(path, error); toolbar_->set_path(path.string()); toolbar_->set_navigation_state(navigation_.can_go_back(), navigation_.can_go_forward());
    if (!error.empty()) { directory_view_->show_error("Unable to display " + path.string() + ": " + error); preview_panel_->show_message("Directory unavailable", error.c_str()); set_status(error); return; }
    directory_view_->show_entries(entries); preview_panel_->show_message("Preview", "Select an item to inspect it.");
    set_status(std::to_string(entries.size()) + " items  |  " + path.string());
}
void FileManagerWindow::activate_entry(const FileEntry& entry) { if (entry.kind == EntryKind::Directory) navigate_to(entry.path); }
void FileManagerWindow::select_entry(const FileEntry& entry) { preview_panel_->show_entry(entry); }
void FileManagerWindow::set_status(const std::string& message) { gtk_label_set_text(GTK_LABEL(status_), message.c_str()); }

GtkWidget* FileManagerWindow::create_menu_bar() {
    GtkWidget* bar = gtk_menu_bar_new(); for (const char* menu : {"File", "Edit", "View", "Bookmarks", "Recent", "Help"}) add_menu(bar, menu); return bar;
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
