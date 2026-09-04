#include "single_instance.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <cerrno>
#include <string>

// Held for the process lifetime and intentionally never closed, so the kernel
// releases the advisory lock automatically on exit/crash.
static int g_lockFd = -1;

bool SingleInstance_Acquire() {
    if (g_lockFd >= 0) return true; // already acquired in this process

    // Per-user lock file: different users on the same machine each get their
    // own goose, but a single user cannot start two.
    std::string path = "/tmp/cadgoose-" + std::to_string((long)getuid()) + ".lock";

    int fd = open(path.c_str(), O_CREAT | O_RDWR, 0600);
    if (fd < 0) {
        // Can't create the lock file (unusual). Fail open: never block startup
        // because of a lock-file problem.
        return true;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        int err = errno;
        close(fd);
        // EWOULDBLOCK/EAGAIN => another live instance holds the lock.
        // Any other error (e.g. flock unsupported on the fs) => fail open.
        return !(err == EWOULDBLOCK || err == EAGAIN);
    }

    g_lockFd = fd; // keep open for the process lifetime
    return true;
}
