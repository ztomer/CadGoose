#include "../test_framework.h"
#include "../../include/cursor_backend.h"

class TestCursorBackend : public CursorBackend {
public:
    std::string Name() const override { return "Test"; }
    uint32_t Caps() const override { return CAP_GET_POS | CAP_MOVE_ABS; }
    bool Init() override { return true; }
};

TEST(CursorBackend_VirtualMethods) {
    TestCursorBackend backend;
    ASSERT_EQ(backend.Name(), "Test");
    ASSERT_EQ(backend.Caps(), CAP_GET_POS | CAP_MOVE_ABS);
    ASSERT_TRUE(backend.Init());
}

TEST(CursorBackendManager_Singleton) {
    // Just verify the global manager exists
    ASSERT_TRUE(g_backendManager.GetActiveBackend() != nullptr);
}

TEST(CursorBackend_DefaultPosition) {
    TestCursorBackend backend;
    Vector2 pos = backend.GetCursorPos();
    ASSERT_EQ(pos.x, -1.0f);
    ASSERT_EQ(pos.y, -1.0f);
}