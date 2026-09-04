#include <gtest/gtest.h>
#include "app_cli.h"
#include <string>
#include <vector>

extern void CommandSocketStub_Reset();
extern void CommandSocketStub_SetResult(bool success);
extern void CommandSocketStub_SetResponse(const std::string& response);
extern void CommandSocketStub_SetError(const std::string& error);
extern std::vector<std::vector<std::string>> CommandSocketStub_GetCommands();

extern bool g_debugMode;

class AppCliTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_debugMode = false;
        CommandSocketStub_Reset();
    }

    int runAppCli(int argc, const char* argv[]) {
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

TEST_F(AppCliTest, VersionFlagReturnsZeroWithoutSocket) {
    const char* argv[] = {"CadGoose", "--version"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 0);
    // --version must exit before touching the running-instance socket.
    EXPECT_TRUE(CommandSocketStub_GetCommands().empty());
}

TEST_F(AppCliTest, VersionShortFlagReturnsZeroWithoutSocket) {
    const char* argv[] = {"CadGoose", "-v"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(CommandSocketStub_GetCommands().empty());
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
    const char* argv[] = {"CadGoose", "--debug", "--mcp"};
    int appArgc = 3;
    char* mut[] = {const_cast<char*>(argv[0]), const_cast<char*>(argv[1]), const_cast<char*>(argv[2])};
    int ret = AppCli_HandleCommand(3, mut, &appArgc);
    EXPECT_EQ(ret, -1);
    EXPECT_TRUE(g_debugMode);
}

TEST_F(AppCliTest, StartForegroundReturnsMinusOne) {
    const char* argv[] = {"CadGoose", "start", "--foreground"};
    int appArgc = 3;
    char* mut[] = {const_cast<char*>(argv[0]), const_cast<char*>(argv[1]), const_cast<char*>(argv[2])};
    int ret = AppCli_HandleCommand(3, mut, &appArgc);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(appArgc, 1);
}

TEST_F(AppCliTest, StartAppAlreadyRunning) {
    CommandSocketStub_SetResult(true);
    CommandSocketStub_SetResponse("goose_count=3");
    const char* argv[] = {"CadGoose", "start"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 0);
    auto cmds = CommandSocketStub_GetCommands();
    ASSERT_GE(cmds.size(), 1u);
    EXPECT_EQ(cmds[0][0], "status");
}

TEST_F(AppCliTest, SpawnViaSocket) {
    CommandSocketStub_SetResult(true);
    CommandSocketStub_SetResponse("ok");
    const char* argv[] = {"CadGoose", "spawn", "TestGoose"};
    int ret = runAppCli(3, argv);
    EXPECT_EQ(ret, 0);
    auto cmds = CommandSocketStub_GetCommands();
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_GE(cmds[0].size(), 2u);
    EXPECT_EQ(cmds[0][0], "spawn");
    EXPECT_EQ(cmds[0][1], "TestGoose");
}

TEST_F(AppCliTest, ClearViaSocket) {
    CommandSocketStub_SetResult(true);
    const char* argv[] = {"CadGoose", "clear"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 0);
    auto cmds = CommandSocketStub_GetCommands();
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0][0], "clear");
}

TEST_F(AppCliTest, RamViaSocket) {
    CommandSocketStub_SetResult(true);
    const char* argv[] = {"CadGoose", "ram"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 0);
    auto cmds = CommandSocketStub_GetCommands();
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0][0], "ram");
}

TEST_F(AppCliTest, StatusViaSocket) {
    CommandSocketStub_SetResult(true);
    CommandSocketStub_SetResponse("goose_count=3");
    const char* argv[] = {"CadGoose", "status"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 0);
    auto cmds = CommandSocketStub_GetCommands();
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0][0], "status");
}

TEST_F(AppCliTest, QuitViaSocket) {
    CommandSocketStub_SetResult(true);
    const char* argv[] = {"CadGoose", "quit"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 0);
    auto cmds = CommandSocketStub_GetCommands();
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0][0], "quit");
}

TEST_F(AppCliTest, FetchViaSocket) {
    CommandSocketStub_SetResult(true);
    const char* argv[] = {"CadGoose", "fetch", "meme"};
    int ret = runAppCli(3, argv);
    EXPECT_EQ(ret, 0);
    auto cmds = CommandSocketStub_GetCommands();
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_GE(cmds[0].size(), 2u);
    EXPECT_EQ(cmds[0][0], "fetch");
    EXPECT_EQ(cmds[0][1], "meme");
}

TEST_F(AppCliTest, DragTestViaSocket) {
    CommandSocketStub_SetResult(true);
    const char* argv[] = {"CadGoose", "drag_test", "100", "200"};
    int ret = runAppCli(4, argv);
    EXPECT_EQ(ret, 0);
    auto cmds = CommandSocketStub_GetCommands();
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0][0], "drag_test");
    EXPECT_EQ(cmds[0][1], "100");
    EXPECT_EQ(cmds[0][2], "200");
}

TEST_F(AppCliTest, SocketFailureReturnsError) {
    CommandSocketStub_SetResult(false);
    CommandSocketStub_SetError("Desktop Goose is not running");
    const char* argv[] = {"CadGoose", "spawn"};
    int ret = runAppCli(2, argv);
    EXPECT_EQ(ret, 1);
}

TEST_F(AppCliTest, BareCallWhenRunningReturnsZero) {
    CommandSocketStub_SetResult(true);
    CommandSocketStub_SetResponse("goose_count=3");
    const char* argv[] = {"CadGoose"};
    int ret = runAppCli(1, argv);
    EXPECT_EQ(ret, 0);
}


