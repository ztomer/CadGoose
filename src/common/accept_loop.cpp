#include "accept_loop.h"

#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

void SetNonBlocking(int fd, bool on) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return;
    fcntl(fd, F_SETFL, on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
}

}  // namespace

bool AcceptLoop::Start(int listenFd, ClientHandler onClient) {
    if (listenFd < 0) return false;
    if (m_running.load()) {
        close(listenFd);
        return false;
    }
    int wake[2];
    if (pipe(wake) != 0) {
        close(listenFd);
        return false;
    }
    // Non-blocking, so a client that resets between poll() and accept() costs
    // an EAGAIN instead of parking the thread where the wake pipe cannot
    // reach it.
    SetNonBlocking(listenFd, true);
    m_listenFd = listenFd;
    m_wakeRead = wake[0];
    m_wakeWrite = wake[1];
    m_running.store(true);
    m_thread = std::thread([this, onClient = std::move(onClient)]() { Run(onClient); });
    return true;
}

void AcceptLoop::Run(ClientHandler onClient) {
    for (;;) {
        pollfd fds[2] = {{m_listenFd, POLLIN, 0}, {m_wakeRead, POLLIN, 0}};
        if (poll(fds, 2, -1) < 0) {
            if (errno == EINTR) continue;
            return;
        }
        if (fds[1].revents != 0) return;  // Stop()
        if ((fds[0].revents & POLLIN) == 0) continue;
        const int clientFd = accept(m_listenFd, nullptr, nullptr);
        if (clientFd < 0) continue;  // EAGAIN / ECONNABORTED: back to poll
        // BSD sockets inherit O_NONBLOCK from the listener; handlers expect
        // ordinary blocking reads and writes.
        SetNonBlocking(clientFd, false);
        onClient(clientFd);
    }
}

void AcceptLoop::Stop() {
    if (!m_running.exchange(false)) return;
    const char byte = 0;
    while (write(m_wakeWrite, &byte, 1) < 0 && errno == EINTR) {
    }
    if (m_thread.joinable()) m_thread.join();
    // Only now, with no thread left inside a call on them.
    close(m_listenFd);
    close(m_wakeRead);
    close(m_wakeWrite);
    m_listenFd = m_wakeRead = m_wakeWrite = -1;
}
