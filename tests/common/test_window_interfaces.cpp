#include <gtest/gtest.h>
#include "i_window_system.h"

extern IWindowSystem* GetWindowSystem();
extern void ResetMockWindowSystem();

class WindowSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        ResetMockWindowSystem();
    }
};

TEST_F(WindowSystemTest, InitialWindowCountIsZero) {
    auto* ws = GetWindowSystem();
    EXPECT_EQ(ws->WindowCount(), 0);
}

TEST_F(WindowSystemTest, CreateWindowIncrementsCount) {
    auto* ws = GetWindowSystem();
    WindowSpec spec;
    spec.x = 100; spec.y = 200;
    spec.width = 300; spec.height = 400;
    spec.level = 0;
    spec.clickThrough = true;
    spec.transparent = true;
    spec.title = "test";

    int id = ws->CreateWindow(spec);
    EXPECT_GT(id, 0);
    EXPECT_EQ(ws->WindowCount(), 1);
}

TEST_F(WindowSystemTest, DestroyWindowDecrementsCount) {
    auto* ws = GetWindowSystem();
    WindowSpec spec;
    int id = ws->CreateWindow(spec);
    EXPECT_EQ(ws->WindowCount(), 1);

    ws->DestroyWindow(id);
    EXPECT_EQ(ws->WindowCount(), 0);
}

TEST_F(WindowSystemTest, UpdatePosition) {
    auto* ws = GetWindowSystem();
    WindowSpec spec;
    spec.x = 0; spec.y = 0;
    spec.width = 100; spec.height = 100;
    int id = ws->CreateWindow(spec);

    ws->UpdatePosition(id, 50, 60, 200, 300);
}

TEST_F(WindowSystemTest, SetVisible) {
    auto* ws = GetWindowSystem();
    int id = ws->CreateWindow({});
    ws->SetVisible(id, false);
    ws->SetVisible(id, true);
}

TEST_F(WindowSystemTest, RequestRedraw) {
    auto* ws = GetWindowSystem();
    int id = ws->CreateWindow({});
    ws->RequestRedraw(id);
}

TEST_F(WindowSystemTest, DestroyNonExistentWindow) {
    auto* ws = GetWindowSystem();
    ws->DestroyWindow(99999);
    EXPECT_EQ(ws->WindowCount(), 0);
}

TEST_F(WindowSystemTest, MultipleWindows) {
    auto* ws = GetWindowSystem();
    int id1 = ws->CreateWindow({});
    int id2 = ws->CreateWindow({});
    int id3 = ws->CreateWindow({});

    EXPECT_EQ(ws->WindowCount(), 3);
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);

    ws->DestroyWindow(id2);
    EXPECT_EQ(ws->WindowCount(), 2);
}
