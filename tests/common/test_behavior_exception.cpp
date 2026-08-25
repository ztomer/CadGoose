// ===========================
// test_behavior_registry.cpp
// Verifies all behaviors are properly registered
// ===========================
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <cstdio>

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"
#include "app_actions.h"
#include "actor.h"
#include <functional>
#include <unistd.h>


// ============================================================
// Exception handling in behavior callbacks
// ============================================================

namespace {
    bool g_throwTestEnabled = false;

    void registerThrowingBehavior(const char* id,
                                  std::function<void(BehaviorContext&)> initFn = nullptr,
                                  std::function<void(Goose*, BehaviorContext&, double, double)> tickFn = nullptr,
                                  std::function<void(Goose*, BehaviorContext&, IRenderer*)> renderFn = nullptr,
                                  std::function<void(BehaviorContext&)> cleanupFn = nullptr) {
        auto bhv = std::make_unique<Behavior>();
        bhv->id = id;
        bhv->enabledPtr = &g_throwTestEnabled;
        bhv->configPtr = &g_throwTestEnabled;
        bhv->init = initFn;
        bhv->tick = tickFn;
        bhv->render = renderFn;
        bhv->cleanup = cleanupFn;
        BehaviorRegistry::Instance().Register(*bhv);
        // ownership escapes
        (void)bhv.release();
    }

    // Capture stderr into a buffer to verify error messages
    struct StderrCapture {
        std::string buffer;
        int oldFd = -1;
        int pipeFds[2] = {-1, -1};
        FILE* oldStderr = nullptr;
        bool active = false;

        void start() {
            if (active) return;
            oldStderr = stderr;
            if (pipe(pipeFds) != 0) return;
            oldFd = dup(fileno(stderr));
            dup2(pipeFds[1], fileno(stderr));
            close(pipeFds[1]);
            active = true;
        }

        std::string stop() {
            if (!active) return buffer;
            fflush(stderr);
            dup2(oldFd, fileno(stderr));
            close(oldFd);
            char buf[4096];
            ssize_t n;
            while ((n = read(pipeFds[0], buf, sizeof(buf) - 1)) > 0) {
                buf[n] = '\0';
                buffer += buf;
            }
            close(pipeFds[0]);
            active = false;
            return buffer;
        }

        ~StderrCapture() { if (active) stop(); }
    };
}

TEST(BehaviorException, InitThrowsInInitAll) {
    g_throwTestEnabled = true;
    registerThrowingBehavior("throw_init", [](BehaviorContext&) {
        throw std::runtime_error("init_exception");
    });

    Goose* g = AppActions_SpawnGoose("TestGoose");
    ASSERT_NE(g, nullptr);

    StderrCapture cap;
    cap.start();
    BehaviorRegistry::Instance().InitAll(g);
    std::string out = cap.stop();

    EXPECT_NE(out.find("Init failed"), std::string::npos) << out;
    EXPECT_NE(out.find("throw_init"), std::string::npos) << out;
    EXPECT_NE(out.find("init_exception"), std::string::npos) << out;

    AppActions_ClearGeese();
    BehaviorRegistry::Instance().Clear();
    BehaviorRegistry::Instance().Restore();
}

TEST(BehaviorException, TickThrowsInTickAll) {
    g_throwTestEnabled = true;
    registerThrowingBehavior("throw_tick", nullptr, [](Goose*, BehaviorContext&, double, double) {
        throw std::runtime_error("tick_exception");
    });

    Goose* g = AppActions_SpawnGoose("TestGoose");
    ASSERT_NE(g, nullptr);
    BehaviorRegistry::Instance().InitAll(g);

    StderrCapture cap;
    cap.start();
    BehaviorRegistry::Instance().TickAll(g, 0.016, 1.0);
    std::string out = cap.stop();

    EXPECT_NE(out.find("Tick failed"), std::string::npos) << out;
    EXPECT_NE(out.find("throw_tick"), std::string::npos) << out;

    AppActions_ClearGeese();
    BehaviorRegistry::Instance().Clear();
    BehaviorRegistry::Instance().Restore();
}

TEST(BehaviorException, RenderThrowsInRenderPass) {
    g_throwTestEnabled = true;
    registerThrowingBehavior("throw_render", nullptr, nullptr, [](Goose*, BehaviorContext&, IRenderer*) {
        throw std::runtime_error("render_exception");
    });

    Goose* g = AppActions_SpawnGoose("TestGoose");
    ASSERT_NE(g, nullptr);
    BehaviorRegistry::Instance().InitAll(g);

    StderrCapture cap;
    cap.start();
    BehaviorRegistry::Instance().RenderAll(g, nullptr);
    std::string out = cap.stop();

    EXPECT_NE(out.find("Render failed"), std::string::npos) << out;
    EXPECT_NE(out.find("throw_render"), std::string::npos) << out;

    AppActions_ClearGeese();
    BehaviorRegistry::Instance().Clear();
    BehaviorRegistry::Instance().Restore();
}

TEST(BehaviorException, CleanupThrowsInCleanupAll) {
    g_throwTestEnabled = true;
    registerThrowingBehavior("throw_cleanup", nullptr, nullptr, nullptr, [](BehaviorContext&) {
        throw std::runtime_error("cleanup_exception");
    });

    Goose* g = AppActions_SpawnGoose("TestGoose");
    ASSERT_NE(g, nullptr);

    // CleanupAll catches with (...) so we can't capture stderr, just verify no crash
    BehaviorRegistry::Instance().CleanupAll(g);

    AppActions_ClearGeese();
    BehaviorRegistry::Instance().Clear();
    BehaviorRegistry::Instance().Restore();
}

TEST(BehaviorException, CleanupThrowsInTickAllTransition) {
    g_throwTestEnabled = true;
    registerThrowingBehavior("throw_cleanup_tick", nullptr, nullptr, nullptr, [](BehaviorContext&) {
        throw std::runtime_error("cleanup_in_tick");
    });

    Goose* g = AppActions_SpawnGoose("TestGoose");
    ASSERT_NE(g, nullptr);
    BehaviorRegistry::Instance().InitAll(g);

    // First tick with enabled=true to set wasEnabled
    BehaviorRegistry::Instance().TickAll(g, 0.016, 1.0);

    // Disable to trigger cleanup transition
    g_throwTestEnabled = false;

    StderrCapture cap;
    cap.start();
    BehaviorRegistry::Instance().TickAll(g, 0.016, 2.0);
    std::string out = cap.stop();

    EXPECT_NE(out.find("Cleanup failed"), std::string::npos) << out;
    EXPECT_NE(out.find("throw_cleanup_tick"), std::string::npos) << out;

    AppActions_ClearGeese();
    BehaviorRegistry::Instance().Clear();
    BehaviorRegistry::Instance().Restore();
}

TEST(BehaviorException, InitThrowsInTickAllTransition) {
    g_throwTestEnabled = false;
    registerThrowingBehavior("throw_init_tick", [](BehaviorContext&) {
        throw std::runtime_error("init_in_tick");
    });

    Goose* g = AppActions_SpawnGoose("TestGoose");
    ASSERT_NE(g, nullptr);

    // Enable to trigger init transition in TickAll
    g_throwTestEnabled = true;

    StderrCapture cap;
    cap.start();
    BehaviorRegistry::Instance().TickAll(g, 0.016, 1.0);
    std::string out = cap.stop();

    EXPECT_NE(out.find("Init failed"), std::string::npos) << out;
    EXPECT_NE(out.find("throw_init_tick"), std::string::npos) << out;

    AppActions_ClearGeese();
    BehaviorRegistry::Instance().Clear();
    BehaviorRegistry::Instance().Restore();
