#include "../test_framework.h"
#include "../../include/cursor_backend.h"
#include "../../include/cursor_io.h"

class TestCursorBackend : public CursorBackend {
public:
    std::string Name() const override { return "Test"; }
    uint32_t Caps() const override { return CAP_GET_POS | CAP_MOVE_ABS; }
    bool Init() override { return true; }
    Vector2 GetCursorPos() override { return m_pos; }
    void MoveCursorAbs(int x, int y) override { m_lastAbsX = x; m_lastAbsY = y; }
    void MoveCursorRel(int dx, int dy) override { m_lastRelX = dx; m_lastRelY = dy; }

    Vector2 m_pos = {-1.0f, -1.0f};
    int m_lastAbsX = 0, m_lastAbsY = 0;
    int m_lastRelX = 0, m_lastRelY = 0;
};

TEST(CursorBackend_VirtualMethods) {
    TestCursorBackend backend;
    ASSERT_EQ(backend.Name(), "Test");
    ASSERT_EQ(backend.Caps(), CAP_GET_POS | CAP_MOVE_ABS);
    ASSERT_TRUE(backend.Init());
}

TEST(CursorBackendManager_Singleton) {
    ASSERT_TRUE(g_backendManager.GetActiveBackend() != nullptr);
    ASSERT_TRUE(g_cursorProvider != nullptr);
}

TEST(CursorBackend_Read) {
    TestCursorBackend backend;
    backend.m_pos = {100.0f, 200.0f};

    CursorState state = backend.Read();
    ASSERT_TRUE(state.hasPos());
    ASSERT_EQ(state.position.x, 100.0f);
    ASSERT_EQ(state.position.y, 200.0f);
    ASSERT_TRUE(state.caps & CAP_GET_POS);
}

TEST(CursorBackend_Execute_MoveAbs) {
    TestCursorBackend backend;

    backend.Execute(CursorAction::MoveAbs(50, 75));
    ASSERT_EQ(backend.m_lastAbsX, 50);
    ASSERT_EQ(backend.m_lastAbsY, 75);
}

TEST(CursorBackend_Execute_MoveRel) {
    TestCursorBackend backend;

    backend.Execute(CursorAction::MoveRel(10, -5));
    ASSERT_EQ(backend.m_lastRelX, 10);
    ASSERT_EQ(backend.m_lastRelY, -5);
}

TEST(CursorState_HasPos) {
    CursorState s1;
    ASSERT_FALSE(s1.hasPos());

    CursorState s2;
    s2.caps = CAP_GET_POS;
    s2.position = {10.0f, 20.0f};
    ASSERT_TRUE(s2.hasPos());

    CursorState s3;
    s3.caps = CAP_MOVE_ABS;
    s3.position = {10.0f, 20.0f};
    ASSERT_FALSE(s3.hasPos());
}