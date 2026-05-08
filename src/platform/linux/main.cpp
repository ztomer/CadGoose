#include <gtk/gtk.h>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>
#include <unistd.h>
#include "app_actions.h"
#include "command_socket.h"
#include "ui.h"
#include "world.h"
#include "config.h"
#include "cursor_backend.h"
#include "app_cli.h"

static void on_activate(GtkApplication* app) {
    static bool initialized = false;
    AppActions_SetApplication(app);

    if (initialized) return;

    Config_Init();
    setup_overlay_window(app);
    g_backendManager.Init();

    std::string error;
    if (!CommandSocket_StartServer(AppActions_HandleCommand, &error) && !error.empty()) {
        std::cerr << error << std::endl;
    }
    AppActions_EnsureInitialGoose();
    initialized = true;
}

int main(int argc, char** argv) {
    char* runArgv[] = { argv[0], nullptr };
    int runArgc = 1;

    const int cliStatus = AppCli_HandleCommand(argc, argv, &runArgc);
    if (cliStatus >= 0) return cliStatus;

    GtkApplication* app = gtk_application_new("com.goose.wayland", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), runArgc, runArgv);
    CommandSocket_StopServer();
    g_object_unref(app);
    return status;
}
