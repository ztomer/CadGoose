#include <gtest/gtest.h>
#include "log.h"
#include <cstdlib>
#include <cstdio>

class LogTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_logLevel = LogLevel::Info;
    }
};

TEST_F(LogTest, InitLevelDebug) {
    unsetenv("CADGOOSE_VERBOSE");
    Log_InitLevel(true);
    EXPECT_EQ(g_logLevel, LogLevel::Debug);
}

TEST_F(LogTest, InitLevelRelease) {
    unsetenv("CADGOOSE_VERBOSE");
    Log_InitLevel(false);
    EXPECT_EQ(g_logLevel, LogLevel::Info);
}

TEST_F(LogTest, InitLevelEnvVarSet) {
    setenv("CADGOOSE_VERBOSE", "1", 1);
    Log_InitLevel(false);
    EXPECT_EQ(g_logLevel, LogLevel::Debug);
}

TEST_F(LogTest, InitLevelEnvVarZero) {
    setenv("CADGOOSE_VERBOSE", "0", 1);
    Log_InitLevel(false);
    EXPECT_EQ(g_logLevel, LogLevel::Info);
}

TEST_F(LogTest, LogvBelowLevelIsNoop) {
    g_logLevel = LogLevel::Info;
    CG_Logv(LogLevel::Debug, "TEST", "should not appear %d", 42);
}

TEST_F(LogTest, LogvAtLevelPrints) {
    g_logLevel = LogLevel::Info;
    CG_Logv(LogLevel::Info, "TEST", "info message %s", "hello");
}

TEST_F(LogTest, LogvErrorPrints) {
    g_logLevel = LogLevel::Info;
    CG_Logv(LogLevel::Error, "TEST", "error: %d", -1);
}

TEST_F(LogTest, LogvWarnPrints) {
    g_logLevel = LogLevel::Warn;
    CG_Logv(LogLevel::Warn, "TEST", "warn");
}

TEST_F(LogTest, LogvMacroError) {
    g_logLevel = LogLevel::Error;
    CG_ERROR("TEST", "macro test %d", 1);
}

TEST_F(LogTest, LogvMacroInfo) {
    g_logLevel = LogLevel::Info;
    CG_INFO("TEST", "info macro");
}

TEST_F(LogTest, LogvMacroDebug) {
    g_logLevel = LogLevel::Debug;
    CG_DEBUG("TEST", "debug macro");
}
