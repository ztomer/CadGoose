#include <gtest/gtest.h>
#include "app_cli.h"

extern bool g_debugMode;

class AppCliTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_debugMode = false;
    }

    int runAppCli(int argc, const char* argv[]) {
        // Build mutable argv as AppCli_HandleCommand expects
        char** mut = new char*[argc];
        for (int i = 0; i < argc; i++) {
            mut[i] = const_cast<char*>(argv[i]);
        }
        int appArgc = argc;
        int ret = AppCli_HandleCommand(argc, mut, &appArgc);
        delete[] mut;
        return ret;
    }
};

TEST_F(AppCliTest, DebugFlagSetsGDebugMode) {
    const char* argv[] = {"CadGoose", "--debug"};
    int ret = runAppCli(2, argv);
    EXPECT_TRUE(g_debugMode);
    // --debug alone returns value from DaemonizeProcess or argc<=1 branch
}

TEST_F(AppCliTest, McpFlagReturnsMinusOne) {
    const char* argv[] = {"CadGoose", "--mcp"};
    int appArgc = 2;
    char* mut[] = {const_cast<char*>(argv[0]), const_cast<char*>(argv[1])};
    int ret = AppCli_HandleCommand(2, mut, &appArgc);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(appArgc, 1);
}

TEST_F(AppCliTest, HelpCommandReturnsZero) {
    const char* argv[] = {"CadGoose", "--help"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 0);
}

TEST_F(AppCliTest, HelpShortCommandReturnsZero) {
    const char* argv[] = {"CadGoose", "help"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 0);
}

TEST_F(AppCliTest, ForegroundFlagReturnsMinusOne) {
    const char* argv[] = {"CadGoose", "--foreground"};
    int appArgc = 2;
    char* mut[] = {const_cast<char*>(argv[0]), const_cast<char*>(argv[1])};
    int ret = AppCli_HandleCommand(2, mut, &appArgc);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(appArgc, 1);
}

TEST_F(AppCliTest, UnknownCommandReturnsMinusOne) {
    const char* argv[] = {"CadGoose", "nonexistent"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, -1);
}

TEST_F(AppCliTest, McpIgnoredWhenNotFirstArg) {
    // --debug before --mcp still processes --mcp
    const char* argv[] = {"CadGoose", "--debug", "--mcp"};
    int appArgc = 3;
    char* mut[] = {const_cast<char*>(argv[0]), const_cast<char*>(argv[1]), const_cast<char*>(argv[2])};
    int ret = AppCli_HandleCommand(3, mut, &appArgc);
    EXPECT_EQ(ret, -1);
    EXPECT_TRUE(g_debugMode);
}
