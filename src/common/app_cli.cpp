#include "app_cli.h"
#include "command_socket.h"
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cstdlib>

#ifdef __APPLE__
#include <spawn.h>
#include <mach-o/dyld.h>
#include <limits.h>
#include <paths.h>
extern char** environ;
#endif

// Declared external to integrate with logging where necessary.
bool g_debugMode = false;

static bool IsControlCommand(const std::string& command) {
    return command == "start" ||
           command == "spawn" ||
           command == "clear" ||
           command == "ram" ||
           command == "status" ||
           command == "quit" ||
           command == "fetch" ||
           command == "drag_test";
}

static bool IsRunning() {
    std::string response;
    return CommandSocket_Send({"status"}, &response, nullptr);
}

static int DaemonizeProcess() {
#ifdef __APPLE__
    // On macOS, fork() corrupts the Objective-C runtime (autorelease pools, etc.).
    // Use posix_spawn to launch a clean child process with --foreground flag.
    char* argv[] = {
        const_cast<char*>("/usr/bin/open"),
        const_cast<char*>("-n"),          // new instance
        const_cast<char*>("-j"),          // hide dock icon (background app)
        const_cast<char*>("-a"),
        const_cast<char*>("CadGoose"),    // app name (requires .app bundle)
        nullptr
    };

    // Fallback: if not in a .app bundle, spawn the binary directly with --foreground
    char exePath[PATH_MAX];
    uint32_t size = sizeof(exePath);
    if (_NSGetExecutablePath(exePath, &size) == 0) {
        char* childArgv[] = {
            exePath,
            const_cast<char*>("--foreground"),
            nullptr
        };
        pid_t pid;
        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        // Redirect stdout/stderr to /dev/null in the child
        int devNull = open("/dev/null", O_RDWR);
        if (devNull >= 0) {
            posix_spawn_file_actions_adddup2(&actions, devNull, STDOUT_FILENO);
            posix_spawn_file_actions_adddup2(&actions, devNull, STDERR_FILENO);
            posix_spawn_file_actions_addclose(&actions, devNull);
        }

        int ret = posix_spawn(&pid, exePath, &actions, nullptr, childArgv, environ);
        if (devNull >= 0) close(devNull);
        posix_spawn_file_actions_destroy(&actions);

        if (ret != 0) {
            std::cerr << "Failed to spawn background process: " << strerror(ret) << std::endl;
            return 1;
        }
        std::cout << "Desktop Goose started in background (PID " << pid << ")" << std::endl;
        return 0;
    }

    std::cerr << "Failed to resolve executable path" << std::endl;
    return 1;
#else
    // Linux: traditional double-fork daemonization
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork background process" << std::endl;
        return 1;
    }

    if (pid > 0) {
        std::cout << "Desktop Goose started in background" << std::endl;
        return 0;
    }

    if (setsid() < 0) _exit(1);

    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) _exit(1);
    if (pid > 0) _exit(0);

    if (chdir("/") != 0) {
        // Keep going even if the cwd cannot be changed.
    }

    const int devNull = open("/dev/null", O_RDWR);
    if (devNull >= 0) {
        dup2(devNull, STDIN_FILENO);
        dup2(devNull, STDOUT_FILENO);
        dup2(devNull, STDERR_FILENO);
        if (devNull > STDERR_FILENO) close(devNull);
    }

    return -1;
#endif
}

int AppCli_HandleCommand(int argc, char** argv, int* appArgc) {
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            g_debugMode = true;
        }
        if (std::string(argv[i]) == "--mcp") {
            *appArgc = 1;
            return -1;
        }
    }

    if (argc <= 1) {
        if (IsRunning()) {
            std::cout << "Desktop Goose is already running" << std::endl;
            return 0;
        }
        return DaemonizeProcess();
    }

    const std::string command = argv[1];
    if (command == "--foreground") {
        *appArgc = 1;
        return -1;
    }

    if (command == "--help" || command == "help") {
        std::cout
            << "Desktop Goose commands:\n"
            << "  CadGoose\n"
            << "  CadGoose --debug\n"
            << "  CadGoose --mcp (run MCP stdio server)\n"
            << "  CadGoose start\n"
            << "  CadGoose start --foreground\n"
            << "  CadGoose spawn [name]\n"
            << "  CadGoose clear\n"
            << "  CadGoose ram\n"
            << "  CadGoose status\n"
            << "  CadGoose fetch [meme|text]\n"
            << "  CadGoose drag_test <x> <y>\n"
            << "  CadGoose quit\n";
        return 0;
    }

    if (!IsControlCommand(command)) return -1;

    if (command == "start") {
        if (argc > 2 && std::string(argv[2]) == "--foreground") {
            *appArgc = 1;
            return -1;
        }

        if (IsRunning()) {
            std::cout << "Desktop Goose is already running" << std::endl;
            return 0;
        }

        return DaemonizeProcess();
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    std::string response;
    std::string error;
    if (!CommandSocket_Send(args, &response, &error)) {
        std::cerr << error << std::endl;
        return 1;
    }

    if (!response.empty()) std::cout << response;
    return 0;
}