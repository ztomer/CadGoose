// AcceptLoop (include/accept_loop.h): one listening socket served by one
// thread, stoppable from another without closing an fd that thread is in.
//
// The four socket servers' own tests reach it only through their happy path;
// these pin its contract directly: what Start() refuses and what it then
// closes, that a client is handed over as a blocking fd, and that Stop() and
// the destructor leave nothing listening. Every socket binds port 0 (G0.3).
#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>

#include "accept_loop.h"

namespace {

// A loopback socket, listening on a port the kernel picks.
int ListenOnAnyPort(int* port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof(addr);
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || listen(fd, 4) != 0 ||
        getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        close(fd);
        return -1;
    }
    *port = ntohs(addr.sin_port);
    return fd;
}

// A connected client socket, or -1 when nothing is listening on the port.
int Connect(int port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = static_cast<in_port_t>(htons(static_cast<uint16_t>(port)));
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

TEST(AcceptLoop, RefusesAnInvalidSocket) {
    AcceptLoop loop;
    EXPECT_FALSE(loop.Start(-1, [](int fd) { close(fd); }));
    EXPECT_FALSE(loop.Running());
    loop.Stop();  // stopping a loop that never ran is a no-op
    EXPECT_FALSE(loop.Running());
}

TEST(AcceptLoop, HandsEachClientOverAsABlockingSocket) {
    int port = 0;
    const int listenFd = ListenOnAnyPort(&port);
    ASSERT_GE(listenFd, 0);

    std::atomic<int> served{0};
    std::atomic<bool> blocking{false};
    AcceptLoop loop;
    ASSERT_TRUE(loop.Start(listenFd, [&](int fd) {
        // The listener is non-blocking; a client must not inherit that.
        blocking = (fcntl(fd, F_GETFL, 0) & O_NONBLOCK) == 0;
        // Counted BEFORE the reply: the client's read of 'k' is what orders
        // this handler's effects before the assertions below. Counted after,
        // the second client could read 'k' and assert while ++served had not
        // run yet (1 failure in ~30 runs; red in a shuffled gate run).
        ++served;
        const char reply = 'k';
        (void)write(fd, &reply, 1);
        close(fd);
    }));
    EXPECT_TRUE(loop.Running());

    for (int i = 0; i < 2; ++i) {
        const int client = Connect(port);
        ASSERT_GE(client, 0);
        char got = 0;
        EXPECT_EQ(read(client, &got, 1), 1);
        EXPECT_EQ(got, 'k');
        close(client);
    }
    EXPECT_EQ(served.load(), 2);
    EXPECT_TRUE(blocking.load());

    loop.Stop();
    EXPECT_FALSE(loop.Running());
    EXPECT_LT(Connect(port), 0) << "Stop() must close the listening socket";
}

TEST(AcceptLoop, ASecondStartIsRefusedAndClosesTheSocketItWasGiven) {
    int port1 = 0, port2 = 0;
    const int first = ListenOnAnyPort(&port1);
    const int second = ListenOnAnyPort(&port2);
    ASSERT_GE(first, 0);
    ASSERT_GE(second, 0);

    AcceptLoop loop;
    ASSERT_TRUE(loop.Start(first, [](int fd) { close(fd); }));
    EXPECT_FALSE(loop.Start(second, [](int fd) { close(fd); }));
    EXPECT_TRUE(loop.Running()) << "the refused Start must not disturb the running loop";
    EXPECT_LT(Connect(port2), 0) << "Start takes ownership, so a refused socket is closed";
    loop.Stop();
}

TEST(AcceptLoop, TheDestructorStopsARunningLoop) {
    int port = 0;
    const int listenFd = ListenOnAnyPort(&port);
    ASSERT_GE(listenFd, 0);
    {
        AcceptLoop loop;
        ASSERT_TRUE(loop.Start(listenFd, [](int fd) { close(fd); }));
    }
    EXPECT_LT(Connect(port), 0);
}

}  // namespace
