#include "app_cli.h"
#include "command_socket.h"
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cstdlib>

// Declared external to integrate with logging where necessary.
bool g_debugMode = false;

static bool IsControlCommand(const std::string& command) {
    return command == "start" ||
           command == "spawn" ||
           command == "clear" ||
           command == "ram" ||
           command == "status" ||
           command == "quit" ||
           command == "fetch";
}

static bool IsRunning() {
    std::string response;
    return CommandSocket_Send({"status"}, &response, nullptr);
}

static int DaemonizeProcess() {
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
    
#ifndef __APPLE__
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
#endif

    return -1;
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