#include <gtk/gtk.h>

#include "file_manager_window.hpp"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    turtle::file_manager::FileManagerWindow window;
    window.show();
    gtk_main();
    return 0;
}
