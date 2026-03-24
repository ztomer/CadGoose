#include "command_socket.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#if defined(__APPLE__)
#define COMMAND_SOCKET_PATH "/tmp/desktop-goose.sock"
#endif

static int g_socketFd = -1;

std::string CommandSocket_GetPath() {
    return COMMAND_SOCKET_PATH;
}

bool CommandSocket_StartServer(CommandHandler handler, std::string* errorOut) {
    g_socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_socketFd < 0) {
        if (errorOut) *errorOut = "Failed to create socket";
        return false;
    }

    unlink(COMMAND_SOCKET_PATH);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, COMMAND_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(g_socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(g_socketFd);
        g_socketFd = -1;
        if (errorOut) *errorOut = "Failed to bind socket";
        return false;
    }

    listen(g_socketFd, 5);
    return true;
}

void CommandSocket_StopServer() {
    if (g_socketFd >= 0) {
        close(g_socketFd);
        g_socketFd = -1;
    }
    unlink(COMMAND_SOCKET_PATH);
}

bool CommandSocket_Send(const std::vector<std::string>& args, std::string* responseOut, std::string* errorOut) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        if (errorOut) *errorOut = "Failed to create socket";
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, COMMAND_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        if (errorOut) *errorOut = "Failed to connect to socket";
        return false;
    }

    std::string msg;
    for (const auto& arg : args) {
        msg += arg + "\n";
    }

    write(fd, msg.c_str(), msg.size());

    char buf[4096] = {0};
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (responseOut && n > 0) {
        responseOut->assign(buf, n);
    }

    return true;
}

bool IsRunning() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return false;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, COMMAND_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    bool running = (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    close(fd);
    return running;
}