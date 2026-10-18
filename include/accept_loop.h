#ifndef CADGOOSE_ACCEPT_LOOP_H
#define CADGOOSE_ACCEPT_LOOP_H

// One listening socket served by one thread, stoppable from another.
//
// The three servers (the internal MCP socket, the MCP HTTP port, the command
// socket) each stopped by closing the listening fd while their thread sat in
// accept() on it. Closing an fd another thread is blocked on is a race the
// kernel does not promise to win: on Darwin the close() waited on that
// accept() forever, and the suite hung in McpServerTest's TearDown on
// iteration 18 of --gtest_repeat=20 (ROADMAP G0.13; both stacks sampled).
// Here the thread waits in poll() on the listening fd AND a wake pipe;
// Stop() writes the pipe, joins the thread, and only then closes anything.

#include <atomic>
#include <functional>
#include <thread>

class AcceptLoop {
public:
    // Runs on the loop thread, once per accepted client, and owns clientFd
    // (a blocking socket).
    using ClientHandler = std::function<void(int clientFd)>;

    AcceptLoop() = default;
    ~AcceptLoop() { Stop(); }
    AcceptLoop(const AcceptLoop&) = delete;
    AcceptLoop& operator=(const AcceptLoop&) = delete;

    // Takes ownership of a bound, listening socket: Stop() closes it, and so
    // does a failed Start(). False if already running or out of fds.
    bool Start(int listenFd, ClientHandler onClient);

    // Wakes the thread, joins it, then closes the listening socket. Safe to
    // call when not running.
    void Stop();

    bool Running() const { return m_running.load(); }

private:
    void Run(ClientHandler onClient);

    std::thread m_thread;
    int m_listenFd = -1;
    int m_wakeRead = -1;
    int m_wakeWrite = -1;
    std::atomic<bool> m_running{false};
};

#endif
