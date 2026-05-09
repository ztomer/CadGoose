#include "command_socket.h"
#include <thread>
#include <atomic>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <mutex>
#include <condition_variable>

#if defined(__APPLE__)
#define COMMAND_SOCKET_PATH "/tmp/desktop-goose.sock"
#endif

namespace {
    CommandHandler g_commandHandler;
    std::thread g_serverThread;
    std::atomic<bool> g_serverRunning{false};
    int g_serverFd = -1;

    std::string EscapeField(const std::string& input) {
        std::string escaped;
        escaped.reserve(input.size());
        for (char c : input) {
            if (c == '\\' || c == '\t' || c == '\n') escaped.push_back('\\');
            escaped.push_back(c == '\n' ? 'n' : c);
        }
        return escaped;
    }

    std::string SerializeArgs(const std::vector<std::string>& args) {
        std::string line;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) line.push_back('\t');
            line += EscapeField(args[i]);
        }
        line.push_back('\n');
        return line;
    }

    std::vector<std::string> ParseArgs(const std::string& line) {
        std::vector<std::string> args;
        std::string current;
        bool escaping = false;

        for (char c : line) {
            if (!escaping) {
                if (c == '\\') {
                    escaping = true;
                    continue;
                }
                if (c == '\t') {
                    args.push_back(current);
                    current.clear();
                    continue;
                }
                if (c == '\n' || c == '\r') continue;
                current.push_back(c);
                continue;
            }
            current.push_back(c == 'n' ? '\n' : c);
            escaping = false;
        }
        args.push_back(current);
        return args;
    }

    void WriteAll(int fd, const std::string& data) {
        size_t written = 0;
        while (written < data.size()) {
            ssize_t rc = write(fd, data.data() + written, data.size() - written);
            if (rc <= 0) return;
            written += (size_t)rc;
        }
    }

    void HandleClientConnection(int clientFd) {
        fprintf(stderr, "[CS-SERVER] HandleClientConnection called\n");
        std::string requestData;
        char buffer[512];
        while (true) {
            ssize_t rc = read(clientFd, buffer, sizeof(buffer));
            fprintf(stderr, "[CS-SERVER] read rc=%zd\n", rc);
            if (rc <= 0) break;
            requestData.append(buffer, (size_t)rc);
        }

        fprintf(stderr, "[CS-SERVER] full request: '%s'\n", requestData.c_str());

        std::string response;
        if (g_commandHandler) {
            std::vector<std::string> args = ParseArgs(requestData);
            fprintf(stderr, "[CS-SERVER] parsed %zu args:", args.size());
            for (size_t i = 0; i < args.size(); ++i) fprintf(stderr, " [%zu]='%s'", i, args[i].c_str());
            fprintf(stderr, "\n");
            response = g_commandHandler(args);
            fprintf(stderr, "[CS-SERVER] handler returned len=%zu: '%s'\n", response.size(), response.c_str());
        } else {
            response = "error command handler unavailable\n";
        }

        fprintf(stderr, "[CS-SERVER] writing response...\n");
        WriteAll(clientFd, response);
        fprintf(stderr, "[CS-SERVER] done\n");
    }
}

std::string CommandSocket_GetPath() {
    return COMMAND_SOCKET_PATH;
}

bool CommandSocket_StartServer(CommandHandler handler, std::string* errorOut) {
    if (g_serverRunning.load()) return true;

    const std::string socketPath = COMMAND_SOCKET_PATH;

    g_serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_serverFd < 0) {
        if (errorOut) *errorOut = "Unable to create command socket";
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    unlink(socketPath.c_str());
    if (bind(g_serverFd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        if (errorOut) *errorOut = "Unable to bind command socket";
        close(g_serverFd);
        g_serverFd = -1;
        return false;
    }

    if (listen(g_serverFd, 8) != 0) {
        if (errorOut) *errorOut = "Unable to listen on command socket";
        close(g_serverFd);
        g_serverFd = -1;
        unlink(socketPath.c_str());
        return false;
    }

    g_commandHandler = std::move(handler);
    g_serverRunning.store(true);

    g_serverThread = std::thread([socketPath]() {
        while (g_serverRunning.load()) {
            struct sockaddr_un clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientFd = accept(g_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientFd < 0) {
                if (!g_serverRunning.load()) break;
                continue;
            }
            HandleClientConnection(clientFd);
            close(clientFd);
        }
        unlink(socketPath.c_str());
    });

    return true;
}

void CommandSocket_StopServer() {
    if (!g_serverRunning.exchange(false)) return;

    if (g_serverFd >= 0) {
        close(g_serverFd);
        g_serverFd = -1;
    }

    if (g_serverThread.joinable()) {
        g_serverThread.join();
    }
}

bool CommandSocket_Send(const std::vector<std::string>& args, std::string* responseOut, std::string* errorOut) {
    const std::string socketPath = COMMAND_SOCKET_PATH;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        if (errorOut) *errorOut = "Failed to create socket";
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        if (errorOut) *errorOut = "Desktop Goose is not running";
        return false;
    }

    std::string msg;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) msg += "\t";
        msg += args[i];
    }
    msg += "\n";

    fprintf(stderr, "[CS-SEND] sending: %s\n", msg.c_str());
    ssize_t w = write(fd, msg.c_str(), msg.size());
    fprintf(stderr, "[CS-SEND] wrote %zd bytes\n", w);
    fprintf(stderr, "[CS-SEND] shutting down write...\n");
    shutdown(fd, SHUT_WR);
    fprintf(stderr, "[CS-SEND] reading...\n");

    char buf[4096] = {0};
    int n = read(fd, buf, sizeof(buf) - 1);
    fprintf(stderr, "[CS-SEND] read %d bytes: '%s'\n", n, buf);
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