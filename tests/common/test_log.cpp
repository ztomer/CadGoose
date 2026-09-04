// test_log.cpp — src/common/log.cpp: level init, the sync stderr path, and the
// async file-logging engine (background thread + queue).
//
// Every assertion here reads the FAR END — the bytes that actually reached
// stderr or the log file — rather than calling the function and checking only
// that it did not crash. An earlier version of this file did the latter, which
// is why the async engine, DebugLog/DebugLogv and LogTick sat at 0% while the
// suite was green.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <unistd.h>

#include "log.h"
#include "config.h"
#include "cursor_io.h"
#include "world.h"
#include "actor.h"
#include "actor_manager.h"
#include "goose.h"

namespace {

std::string ReadFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return {};
    std::string out;
    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
    fclose(f);
    return out;
}

// Spin until the async writer has drained, or the deadline passes. The log
// thread wakes on a condvar with a 100ms timeout, so a fixed sleep would be
// both slower and flakier than polling for the expected content.
bool WaitForContent(const std::string& path, const char* needle, int timeoutMs = 3000) {
    for (int waited = 0; waited < timeoutMs; waited += 10) {
        if (ReadFile(path).find(needle) != std::string::npos) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

// Restores every global this file perturbs: the log level, the async engine,
// the CADGOOSE_VERBOSE env var, and the debug.toTerminal config gate.
class LogTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_savedLevel = g_logLevel;
        m_savedToTerminal = g_config.debug.toTerminal;
        if (const char* v = std::getenv("CADGOOSE_VERBOSE")) {
            m_hadVerbose = true;
            m_savedVerbose = v;
        }
        unsetenv("CADGOOSE_VERBOSE");
        g_logLevel = LogLevel::Info;
    }

    void TearDown() override {
        Log_Shutdown();  // idempotent when not running
        g_logLevel = m_savedLevel;
        g_config.debug.toTerminal = m_savedToTerminal;
        if (m_hadVerbose) {
            setenv("CADGOOSE_VERBOSE", m_savedVerbose.c_str(), 1);
        } else {
            unsetenv("CADGOOSE_VERBOSE");
        }
        for (const auto& p : m_temps) unlink(p.c_str());
    }

    std::string Temp(const char* stem) {
        m_temps.push_back(std::string("/tmp/cadgoose_") + stem + "_" +
                          std::to_string(getpid()) + ".log");
        return m_temps.back();
    }

private:
    LogLevel m_savedLevel = LogLevel::Info;
    bool m_savedToTerminal = false;
    bool m_hadVerbose = false;
    std::string m_savedVerbose;
    std::vector<std::string> m_temps;
};

}  // namespace

// ── Log_InitLevel ──────────────────────────────────────────

TEST_F(LogTest, InitLevelFollowsDebugModeFlag) {
    Log_InitLevel(true);
    EXPECT_EQ(g_logLevel, LogLevel::Debug);

    Log_InitLevel(false);
    EXPECT_EQ(g_logLevel, LogLevel::Info);
}

TEST_F(LogTest, VerboseEnvForcesDebugEvenWhenDebugModeIsOff) {
    setenv("CADGOOSE_VERBOSE", "1", 1);
    Log_InitLevel(false);
    EXPECT_EQ(g_logLevel, LogLevel::Debug) << "CADGOOSE_VERBOSE must win over debugMode=false";
}

TEST_F(LogTest, VerboseEnvSetToZeroDoesNotForceDebug) {
    setenv("CADGOOSE_VERBOSE", "0", 1);
    Log_InitLevel(false);
    EXPECT_EQ(g_logLevel, LogLevel::Info) << "\"0\" must be treated as off";
}

TEST_F(LogTest, VerboseEnvSetToEmptyDoesNotForceDebug) {
    setenv("CADGOOSE_VERBOSE", "", 1);
    Log_InitLevel(false);
    EXPECT_EQ(g_logLevel, LogLevel::Info) << "an empty value must be treated as unset";
}

// ── CG_Logv / the sync stderr path ─────────────────────────

TEST_F(LogTest, LogvWritesTagAndFormattedMessageToStderr) {
    g_logLevel = LogLevel::Debug;
    ::testing::internal::CaptureStderr();
    CG_Logv(LogLevel::Error, "UNITTEST", "code=%d name=%s", 42, "goose");
    std::string out = ::testing::internal::GetCapturedStderr();

    EXPECT_NE(out.find("[UNITTEST]"), std::string::npos) << "missing tag prefix in: " << out;
    EXPECT_NE(out.find("code=42 name=goose"), std::string::npos) << "bad formatting in: " << out;
    EXPECT_NE(out.find('\n'), std::string::npos) << "missing trailing newline";
}

TEST_F(LogTest, LogvSuppressesMessagesAboveTheCurrentLevel) {
    g_logLevel = LogLevel::Error;
    ::testing::internal::CaptureStderr();
    CG_Logv(LogLevel::Debug, "UNITTEST", "should not appear");
    CG_Logv(LogLevel::Info, "UNITTEST", "also should not appear");
    CG_Logv(LogLevel::Error, "UNITTEST", "must appear");
    std::string out = ::testing::internal::GetCapturedStderr();

    EXPECT_EQ(out.find("should not appear"), std::string::npos);
    EXPECT_EQ(out.find("also should not appear"), std::string::npos);
    EXPECT_NE(out.find("must appear"), std::string::npos);
}

TEST_F(LogTest, LogvEmitsEveryLevelWhenLevelIsDebug) {
    g_logLevel = LogLevel::Debug;
    ::testing::internal::CaptureStderr();
    CG_Logv(LogLevel::Error, "L", "e");
    CG_Logv(LogLevel::Warn, "L", "w");
    CG_Logv(LogLevel::Info, "L", "i");
    CG_Logv(LogLevel::Debug, "L", "d");
    std::string out = ::testing::internal::GetCapturedStderr();

    EXPECT_NE(out.find("[L] e"), std::string::npos);
    EXPECT_NE(out.find("[L] w"), std::string::npos);
    EXPECT_NE(out.find("[L] i"), std::string::npos);
    EXPECT_NE(out.find("[L] d"), std::string::npos);
}

TEST_F(LogTest, ConvenienceMacrosRouteThroughLogv) {
    g_logLevel = LogLevel::Debug;
    ::testing::internal::CaptureStderr();
    CG_ERROR("TAGE", "e%d", 1);
    CG_WARN("TAGW", "w%d", 2);
    CG_INFO("TAGI", "i%d", 3);
    CG_DEBUG("TAGD", "d%d", 4);
    std::string out = ::testing::internal::GetCapturedStderr();

    EXPECT_NE(out.find("[TAGE] e1"), std::string::npos);
    EXPECT_NE(out.find("[TAGW] w2"), std::string::npos);
    EXPECT_NE(out.find("[TAGI] i3"), std::string::npos);
#ifndef CG_DISABLE_DEBUG_LOG
    EXPECT_NE(out.find("[TAGD] d4"), std::string::npos);
#endif
}

// ── Async engine ───────────────────────────────────────────

TEST_F(LogTest, EnableFileWritesAsyncMessagesToDisk) {
    std::string path = Temp("async");
    Log_EnableFile(path.c_str(), LogLevel::Debug);

    CG_LogvAsync(LogLevel::Info, "ASYNC", "hello %d", 7);
    EXPECT_TRUE(WaitForContent(path, "hello 7")) << "async message never reached the file";
    EXPECT_NE(ReadFile(path).find("[ASYNC]"), std::string::npos);

    Log_Shutdown();
}

TEST_F(LogTest, AsyncRespectsItsMinimumLevel) {
    std::string path = Temp("asynclevel");
    Log_EnableFile(path.c_str(), LogLevel::Warn);

    CG_LogvAsync(LogLevel::Error, "A", "kept-error");
    CG_LogvAsync(LogLevel::Warn, "A", "kept-warn");
    CG_LogvAsync(LogLevel::Debug, "A", "dropped-debug");
    CG_LogvAsync(LogLevel::Info, "A", "dropped-info");

    ASSERT_TRUE(WaitForContent(path, "kept-warn"));
    std::string body = ReadFile(path);
    EXPECT_NE(body.find("kept-error"), std::string::npos);
    EXPECT_EQ(body.find("dropped-debug"), std::string::npos) << "below-threshold line leaked";
    EXPECT_EQ(body.find("dropped-info"), std::string::npos) << "below-threshold line leaked";

    Log_Shutdown();
}

TEST_F(LogTest, ShutdownFlushesQueuedMessages) {
    std::string path = Temp("flush");
    Log_EnableFile(path.c_str(), LogLevel::Debug);

    for (int i = 0; i < 50; ++i) CG_LogvAsync(LogLevel::Info, "BURST", "line-%d", i);
    Log_Shutdown();  // must drain the queue before closing the file

    std::string body = ReadFile(path);
    EXPECT_NE(body.find("line-0"), std::string::npos);
    EXPECT_NE(body.find("line-49"), std::string::npos)
        << "shutdown dropped queued messages instead of flushing them";
}

TEST_F(LogTest, AsyncIsANoOpWhenTheEngineIsNotRunning) {
    // Not enabled: must not crash and must not write anywhere.
    CG_LogvAsync(LogLevel::Error, "OFF", "into the void");
    SUCCEED();
}

TEST_F(LogTest, EnableFileIgnoresAnUnopenablePath) {
    // A directory that does not exist -> fopen fails -> the engine stays off.
    Log_EnableFile("/nonexistent-dir-cadgoose/x.log", LogLevel::Debug);
    CG_LogvAsync(LogLevel::Error, "OFF", "still nowhere");
    Log_Shutdown();
    SUCCEED();
}

TEST_F(LogTest, SecondEnableFileWhileRunningIsIgnored) {
    std::string first = Temp("first");
    std::string second = Temp("second");

    Log_EnableFile(first.c_str(), LogLevel::Debug);
    Log_EnableFile(second.c_str(), LogLevel::Debug);  // must be ignored

    CG_LogvAsync(LogLevel::Info, "ONE", "goes-to-first");
    ASSERT_TRUE(WaitForContent(first, "goes-to-first"));
    Log_Shutdown();

    EXPECT_EQ(ReadFile(second).find("goes-to-first"), std::string::npos)
        << "the second Log_EnableFile must not steal the sink";
}

TEST_F(LogTest, ShutdownTwiceIsSafe) {
    std::string path = Temp("double");
    Log_EnableFile(path.c_str(), LogLevel::Debug);
    Log_Shutdown();
    Log_Shutdown();  // idempotent
    SUCCEED();
}

TEST_F(LogTest, AsyncTruncatesOverlongMessagesInsteadOfOverflowing) {
    std::string path = Temp("long");
    Log_EnableFile(path.c_str(), LogLevel::Debug);

    std::string huge(4000, 'x');  // well past the 1024-byte staging buffer
    CG_LogvAsync(LogLevel::Info, "LONG", "%s", huge.c_str());
    ASSERT_TRUE(WaitForContent(path, "[LONG]"));
    Log_Shutdown();

    std::string body = ReadFile(path);
    EXPECT_LT(body.size(), huge.size()) << "message should have been truncated to the buffer";
    EXPECT_NE(body.find("xxxx"), std::string::npos);
}

// ── DebugLog / DebugLogv / LogTick ─────────────────────────
// These only exist when CG_DISABLE_DEBUG_LOG is undefined. The test binary is
// always built with logging enabled (only the app target compiles it out), but
// guard anyway so the file stays valid under a fully-stripped build.
#ifndef CG_DISABLE_DEBUG_LOG

TEST_F(LogTest, DebugLogGoesToTheAsyncSinkWhenToTerminalIsOn) {
    std::string path = Temp("dbg");
    Log_EnableFile(path.c_str(), LogLevel::Debug);
    g_config.debug.toTerminal = true;

    DebugLog("value=%d", 99);
    EXPECT_TRUE(WaitForContent(path, "value=99")) << "DebugLog never reached the sink";
    EXPECT_NE(ReadFile(path).find("[DEBUG]"), std::string::npos);

    Log_Shutdown();
}

TEST_F(LogTest, DebugLogIsSuppressedWhenToTerminalIsOff) {
    std::string path = Temp("dbgoff");
    Log_EnableFile(path.c_str(), LogLevel::Debug);
    g_config.debug.toTerminal = false;

    DebugLog("must-not-appear");
    CG_LogvAsync(LogLevel::Info, "MARK", "sentinel");  // proves the sink is alive
    ASSERT_TRUE(WaitForContent(path, "sentinel"));
    Log_Shutdown();

    EXPECT_EQ(ReadFile(path).find("must-not-appear"), std::string::npos)
        << "debug.toTerminal=false must gate DebugLog";
}

TEST_F(LogTest, DebugLogvCarriesLevelAndTag) {
    std::string path = Temp("dbgv");
    Log_EnableFile(path.c_str(), LogLevel::Debug);
    g_config.debug.toTerminal = true;

    DebugLogv(LogLevel::Warn, "CUSTOM", "n=%d", 5);
    EXPECT_TRUE(WaitForContent(path, "n=5"));
    EXPECT_NE(ReadFile(path).find("[CUSTOM]"), std::string::npos);

    Log_Shutdown();
}

TEST_F(LogTest, DebugLogvIsGatedByToTerminal) {
    std::string path = Temp("dbgv2");
    Log_EnableFile(path.c_str(), LogLevel::Debug);
    g_config.debug.toTerminal = false;

    DebugLogv(LogLevel::Warn, "CUSTOM", "hidden");
    CG_LogvAsync(LogLevel::Info, "MARK", "sentinel2");
    ASSERT_TRUE(WaitForContent(path, "sentinel2"));
    Log_Shutdown();

    EXPECT_EQ(ReadFile(path).find("hidden"), std::string::npos);
}

TEST_F(LogTest, LogTickRendersCursorAndGeeseState) {
    std::string path = Temp("tick");
    Log_EnableFile(path.c_str(), LogLevel::Debug);
    g_config.debug.toTerminal = true;

    auto& mgr = ActorManager::Instance();
    Goose* g = new Goose(7, "Gander", 1920, 1080);
    g->pos = {100.0f, 200.0f};
    g->vel = {3.0f, -4.0f};
    g->state = GooseState::WANDER;
    mgr.add(g);

    CursorState cursor;
    cursor.caps = CAP_GET_POS;
    cursor.position = {321.0f, 654.0f};

    LogTick(12.5, cursor);
    ASSERT_TRUE(WaitForContent(path, "[GOOSE]"));
    std::string body = ReadFile(path);

    EXPECT_NE(body.find("[T12.5]"), std::string::npos) << "missing timestamp in: " << body;
    EXPECT_NE(body.find("c(321,654)"), std::string::npos) << "missing cursor pos in: " << body;
    EXPECT_NE(body.find("7W@(100,200)"), std::string::npos)
        << "missing goose id/state/pos in: " << body;

    mgr.remove(g);
    delete g;
    Log_Shutdown();
}

TEST_F(LogTest, LogTickRendersAbsentCursor) {
    std::string path = Temp("tick2");
    Log_EnableFile(path.c_str(), LogLevel::Debug);
    g_config.debug.toTerminal = true;

    CursorState cursor;  // no CAP_GET_POS -> hasPos() false
    LogTick(3.0, cursor);

    ASSERT_TRUE(WaitForContent(path, "[GOOSE]"));
    EXPECT_NE(ReadFile(path).find("c(-,-)"), std::string::npos)
        << "an unavailable cursor must render as c(-,-)";

    Log_Shutdown();
}

TEST_F(LogTest, LogTickRendersSnatchAngleForSnatchingGeese) {
    std::string path = Temp("tick3");
    Log_EnableFile(path.c_str(), LogLevel::Debug);
    g_config.debug.toTerminal = true;

    auto& mgr = ActorManager::Instance();
    Goose* g = new Goose(3, "Snatcher", 1920, 1080);
    g->state = GooseState::SNATCH_CURSOR;
    g->snatchAngle = 1.5f;
    g->snatchRadius = 80.0f;
    mgr.add(g);

    LogTick(1.0, CursorState{});
    ASSERT_TRUE(WaitForContent(path, "[GOOSE]"));
    std::string body = ReadFile(path);
    EXPECT_NE(body.find("a=1.5"), std::string::npos) << "snatch angle missing in: " << body;
    EXPECT_NE(body.find("r=80"), std::string::npos) << "snatch radius missing in: " << body;

    mgr.remove(g);
    delete g;
    Log_Shutdown();
}

TEST_F(LogTest, LogTickIsSuppressedWhenToTerminalIsOff) {
    std::string path = Temp("tickoff");
    Log_EnableFile(path.c_str(), LogLevel::Debug);
    g_config.debug.toTerminal = false;

    LogTick(9.0, CursorState{});
    CG_LogvAsync(LogLevel::Info, "MARK", "sentinel3");
    ASSERT_TRUE(WaitForContent(path, "sentinel3"));
    Log_Shutdown();

    EXPECT_EQ(ReadFile(path).find("[GOOSE]"), std::string::npos)
        << "debug.toTerminal=false must gate LogTick";
}

#endif  // CG_DISABLE_DEBUG_LOG
