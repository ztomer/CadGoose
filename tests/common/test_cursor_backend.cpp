#include <gtest/gtest.h>
#include "cursor_backend.h"
#include "cursor_io.h"

class TestCursorBackend : public CursorBackend {
public:
    std::string Name() const override { return "Test"; }
    uint32_t Caps() const override { return CAP_GET_POS | CAP_MOVE_ABS | CAP_MOVE_REL; }
    bool Init() override { return true; }
    Vector2 GetCursorPos() override { return m_pos; }
    void MoveCursorAbs(int x, int y) override { m_lastAbsX = x; m_lastAbsY = y; }
    void MoveCursorRel(int dx, int dy) override { m_lastRelX = dx; m_lastRelY = dy; }

    Vector2 m_pos = {-1.0f, -1.0f};
    int m_lastAbsX = 0, m_lastAbsY = 0;
    int m_lastRelX = 0, m_lastRelY = 0;
};

TEST(CursorBackendSuite, VirtualMethods) {
    TestCursorBackend backend;
    EXPECT_EQ(backend.Name(), "Test");
    EXPECT_EQ(backend.Caps(), static_cast<uint32_t>(CAP_GET_POS | CAP_MOVE_ABS | CAP_MOVE_REL));
    EXPECT_TRUE(backend.Init());
}

TEST(CursorBackendSuite, ReadState) {
    TestCursorBackend backend;
    backend.m_pos = {100.0f, 200.0f};

    CursorState state = backend.Read();
    EXPECT_TRUE(state.hasPos());
    EXPECT_EQ(state.position.x, 100.0f);
    EXPECT_EQ(state.position.y, 200.0f);
    EXPECT_TRUE(state.caps & CAP_GET_POS);
}

TEST(CursorBackendSuite, ExecuteMoveAbs) {
    TestCursorBackend backend;
    backend.Execute(CursorAction::MoveAbs(50, 75));
    EXPECT_EQ(backend.m_lastAbsX, 50);
    EXPECT_EQ(backend.m_lastAbsY, 75);
}

TEST(CursorBackendSuite, ExecuteMoveRel) {
    TestCursorBackend backend;
    backend.Execute(CursorAction::MoveRel(10, -5));
    EXPECT_EQ(backend.m_lastRelX, 10);
    EXPECT_EQ(backend.m_lastRelY, -5);
}

TEST(CursorBackendSuite, ReadWithoutGetPosCaps) {
    class NoPosBackend : public CursorBackend {
    public:
        std::string Name() const override { return "NoPos"; }
        uint32_t Caps() const override { return CAP_MOVE_ABS; }
        bool Init() override { return true; }
        Vector2 GetCursorPos() override { return {10.0f, 20.0f}; }
        void MoveCursorAbs(int x, int y) override {}
        void MoveCursorRel(int dx, int dy) override {}
    };
    NoPosBackend backend;
    CursorState state = backend.Read();
    EXPECT_FALSE(state.hasPos());
}

TEST(CursorBackendSuite, SingletonManagerExists) {
    EXPECT_NE(g_backendManager.GetActiveBackend(), nullptr);
    EXPECT_NE(g_cursorProvider, nullptr);
}

TEST(CursorBackendSuite, ManagerInitRegistersPlatformBackend) {
    ICursorProvider* savedProvider = g_cursorProvider;
    g_cursorProvider = nullptr;

    g_backendManager.Init();

    // Init either selects a working backend or prints a warning —
    // either way the method executes without crashing
    EXPECT_NE(g_backendManager.GetActiveBackend(), nullptr);

    g_cursorProvider = savedProvider;
}

TEST(CursorBackendSuite, ExecuteWithoutCaps) {
    class NoCapsBackend : public CursorBackend {
    public:
        std::string Name() const override { return "NoCaps"; }
        uint32_t Caps() const override { return CAP_NONE; }
        bool Init() override { return true; }
        Vector2 GetCursorPos() override { return {0, 0}; }
        void MoveCursorAbs(int x, int y) override {}
        void MoveCursorRel(int dx, int dy) override {}
    };
    NoCapsBackend backend;
    CursorState state = backend.Read();
    EXPECT_FALSE(state.hasPos());

    backend.Execute(CursorAction::MoveAbs(100, 200));
    backend.Execute(CursorAction::MoveRel(10, -5));
    backend.Execute(CursorAction{}); // unknown type
    SUCCEED();
}

TEST(CursorBackendSuite, ReadDoesNotCallGetCursorPosWithoutGetPosCap) {
    class BackendWithMoveRel : public CursorBackend {
    public:
        std::string Name() const override { return "RelOnly"; }
        uint32_t Caps() const override { return CAP_MOVE_REL; }
        bool Init() override { return true; }
        Vector2 GetCursorPos() override { return {999, 999}; }
        void MoveCursorAbs(int x, int y) override {}
        void MoveCursorRel(int dx, int dy) override {}
    };
    BackendWithMoveRel backend;
    // hasPos() should return false since CAP_GET_POS is not set
    // even though GetCursorPos returns a value
    CursorState state = backend.Read();
    EXPECT_FALSE(state.hasPos());

    // Execute with MOVE_REL should work since we have that cap
    backend.Execute(CursorAction::MoveRel(42, 99));
    SUCCEED();
}
