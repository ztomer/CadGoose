#include "command_socket.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

#if defined(__APPLE__)
#define COMMAND_SOCKET_PATH "/tmp/desktop-goose.sock"
#endif

static int g_socketFd = -1;

bool CommandSocket_Init() {
    g_socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_socketFd < 0) return false;

    unlink(COMMAND_SOCKET_PATH);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, COMMAND_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(g_socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(g_socketFd);
        g_socketFd = -1;
        return false;
    }

    listen(g_socketFd, 5);
    return true;
}

int CommandSocket_Send(const std::vector<std::string>& args, std::string* response, int* statusCode) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, COMMAND_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    std::string msg;
    for (const auto& arg : args) {
        msg += arg + "\n";
    }

    write(fd, msg.c_str(), msg.size());

    char buf[4096] = {0};
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (response && n > 0) {
        response->assign(buf, n);
    }

    return 0;
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